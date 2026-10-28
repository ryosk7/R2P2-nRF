#!/usr/bin/env ruby
# Turn a CURRENT.UF2 read off the bootloader drive into a UF2 that puts
# a range of flash back exactly as it was.
#
# CURRENT.UF2 is a byte-exact snapshot of the application space,
# SoftDevice included, so it is the way back from a Thread build that
# does not depend on a release hex matching what is on the board. It
# cannot be dropped back on as it is when only part of it is wanted:
# every block carries blockNo/numBlocks for the whole file, and the
# bootloader finishes -- flushes the last page and marks the bank valid
# -- only once it has counted numBlocks distinct blocks. A subset keeps
# the original numbers and never completes. So the blocks are renumbered
# 0..N-1 with numBlocks = N.
#
# Erased (0xFF) blocks are kept, not dropped as a hex-to-UF2 conversion
# would: they are what overwrites whatever a later image left behind.
#
# Usage: uf2_restore.rb CURRENT.UF2 OUT.UF2 [START [END]]
#   START/END default to 0x1000..0xEA000, the whole range the bootloader
#   exposes and accepts on this board.

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END    = 0x0AB16F30
UF2_FLAG_NOFLASH  = 0x00000001
UF2_FLAG_FAMILYID = 0x00002000
BLOCK_SIZE = 512

src, out, start_arg, end_arg = ARGV
abort "usage: #{$0} CURRENT.UF2 OUT.UF2 [START [END]]" unless src && out
range_start = start_arg ? Integer(start_arg) : 0x1000
range_end   = end_arg ? Integer(end_arg) : 0xEA000

data = File.binread(src)
abort "#{src}: size #{data.bytesize} is not a multiple of #{BLOCK_SIZE}" unless data.bytesize % BLOCK_SIZE == 0

blocks = []
family = nil
payload_size = nil
i = 0
count = data.bytesize / BLOCK_SIZE
while i < count
  raw = data.byteslice(i * BLOCK_SIZE, BLOCK_SIZE)
  m0, m1, flags, addr, size, _no, _num, fam = raw.unpack('L<8')
  m_end = raw.byteslice(BLOCK_SIZE - 4, 4).unpack1('L<')
  unless m0 == UF2_MAGIC_START0 && m1 == UF2_MAGIC_START1 && m_end == UF2_MAGIC_END
    abort "#{src}: block #{i} is not a UF2 block"
  end
  abort "#{src}: block #{i} is marked NOFLASH" if flags & UF2_FLAG_NOFLASH != 0
  abort "#{src}: block #{i} carries no family ID" if flags & UF2_FLAG_FAMILYID == 0
  family ||= fam
  abort "#{src}: block #{i} has family 0x#{fam.to_s(16)}, expected 0x#{family.to_s(16)}" unless fam == family
  payload_size ||= size
  abort "#{src}: block #{i} has payload size #{size}, expected #{payload_size}" unless size == payload_size
  if addr >= range_start && addr + size <= range_end
    blocks << [addr, raw.byteslice(32, size)]
  end
  i += 1
end

blocks.sort_by! { |b| b[0] }

# Every address in the range must be covered exactly once; a gap would
# leave stale bytes behind, which is the one thing this file is for.
expected = range_start
k = 0
nblocks = blocks.size
while k < nblocks
  addr = blocks[k][0]
  abort "gap or overlap at 0x#{expected.to_s(16)} (next block at 0x#{addr.to_s(16)})" unless addr == expected
  expected += payload_size
  k += 1
end
abort "range ends at 0x#{expected.to_s(16)}, wanted 0x#{range_end.to_s(16)}" unless expected == range_end

File.open(out, 'wb') do |f|
  k = 0
  while k < nblocks
    addr, payload = blocks[k]
    header = [UF2_MAGIC_START0, UF2_MAGIC_START1, UF2_FLAG_FAMILYID,
              addr, payload_size, k, nblocks, family].pack('L<8')
    f.write(header + payload.ljust(476, "\x00") + [UF2_MAGIC_END].pack('L<'))
    k += 1
  end
end

printf("%s: %d blocks, 0x%x..0x%x, family 0x%08x\n", out, nblocks, range_start, range_end, family)
