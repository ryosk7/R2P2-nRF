#!/usr/bin/env ruby
# Merge the SoftDevice out of a bootloader release hex with a firmware hex.
#
# Flashing a firmware built for the no-SoftDevice map erases the
# SoftDevice, and that is one-way: dropping a SoftDevice-layout .uf2 back
# on afterwards does not recover, because the bootloader starts the app
# at MBR_SIZE where the previous image's vector table still sits, and
# restoring the SoftDevice alone leaves stale bytes that the UF2 path
# happily marks as a valid application.
#
# The way back is one image containing both, converted in a single pass:
# two concatenated .uf2 files do not work either, because the bootloader
# invalidates numBlocks when it sees a second value.
#
# Usage: merge_sd_hex.rb BOOTLOADER_HEX FIRMWARE_HEX OUT_HEX
#   Emits only the SoftDevice range from BOOTLOADER_HEX, so the MBR (which
#   is already on the device) and the bootloader region (which the UF2
#   path refuses) are left out.

SD_START = 0x1000
SD_END   = 0x26000   # SD_SIZE for S140 6.1.1

def each_record(path)
  base = 0
  File.foreach(path) do |line|
    line = line.strip
    next unless line.start_with?(":")
    len  = line[1, 2].to_i(16)
    addr = line[3, 4].to_i(16)
    type = line[7, 2].to_i(16)
    case type
    when 4 then base = line[9, 4].to_i(16) << 16
    when 0 then yield(base + addr, line)
    end
    yield(nil, line) if type == 4
  end
end

bootloader, firmware, out = ARGV
abort "usage: merge_sd_hex.rb BOOTLOADER_HEX FIRMWARE_HEX OUT_HEX" unless out

File.open(out, "w") do |f|
  # SoftDevice only. Extended-address records are re-emitted rather than
  # copied: the bootloader hex carries one for UICR at 0x10001000, and
  # passing it through would leave that base in effect and shift every
  # firmware record that follows by 0x10000000.
  emitted_base = nil
  each_record(bootloader) do |addr, line|
    next if addr.nil?
    next unless addr >= SD_START && addr < SD_END
    base = addr >> 16
    if base != emitted_base
      hi = (base >> 8) & 0xFF
      lo = base & 0xFF
      sum = (-(2 + 0 + 0 + 4 + hi + lo)) & 0xFF
      f.puts format(":02000004%02X%02X%02X", hi, lo, sum)
      emitted_base = base
    end
    f.puts line
  end
  # Reset the base before the firmware, which carries its own records.
  f.puts ":020000040000FA"
  # Then the firmware verbatim, minus its EOF record.
  File.foreach(firmware) do |line|
    line = line.strip
    next if line.start_with?(":00000001")
    f.puts line
  end
  f.puts ":00000001FF"
end
