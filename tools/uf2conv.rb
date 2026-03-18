#!/usr/bin/env ruby
require 'optparse'
require 'json'

UF2_MAGIC_START0 = 0x0A324655 # "UF2\n"
UF2_MAGIC_START1 = 0x9E5D5157 # Randomly selected
UF2_MAGIC_END    = 0x0AB16F30 # Ditto

class Block
  attr_accessor :addr, :bytes

  def initialize(addr)
    @addr = addr
    @bytes = Array.new(256, 0)
  end

  def encode(blockno, numblocks, family_id)
    flags = 0x0
    flags |= 0x2000 if family_id != 0

    header = [
      UF2_MAGIC_START0,
      UF2_MAGIC_START1,
      flags,
      @addr,
      256,
      blockno,
      numblocks,
      family_id
    ].pack('L<*')

    header + @bytes.pack('C*').ljust(476, "\x00") + [UF2_MAGIC_END].pack('L<')
  end
end

def convert_from_hex_to_uf2(hex_content, family_id)
  app_start_addr = nil
  upper = 0
  curr_block = nil
  blocks = []

  hex_content.each_line do |line|
    line.strip!
    next if line.length < 11 || line[0] != ':'

    bytes = line[1..-3].scan(/../).map { |s| s.to_i(16) }
    # checksum = line[-2..-1].to_i(16)
    # calculated_checksum = (0x100 - (bytes.reduce(:+) & 0xFF)) & 0xFF
    # next if checksum != calculated_checksum

    byte_count = bytes[0]
    addr = (bytes[1] << 8) | bytes[2]
    rec_type = bytes[3]

    case rec_type
    when 4 # Extended Linear Address
      upper = ((bytes[4] << 8) | bytes[5]) << 16
    when 2 # Extended Segment Address
      upper = ((bytes[4] << 8) | bytes[5]) << 4
    when 1 # End Of File
      break
    when 0 # Data
      data_addr = upper + addr
      app_start_addr ||= data_addr
      data = bytes[4..-1]

      data.each_with_index do |byte, i|
        addr_ptr = data_addr + i
        block_addr = addr_ptr & ~0xff

        if !curr_block || curr_block.addr != block_addr
          curr_block = Block.new(block_addr)
          blocks << curr_block
        end
        curr_block.bytes[addr_ptr & 0xff] = byte
      end
    end
  end

  num_blocks = blocks.length
  resfile = blocks.each_with_index.map do |block, i|
    block.encode(i, num_blocks, family_id)
  end.join

  return resfile, app_start_addr
end

def load_families
  filename = "uf2families.json"
  pathname = File.join(File.dirname(File.expand_path(__FILE__)), filename)
  families = {}
  JSON.parse(File.read(pathname)).each do |f|
    families[f["short_name"]] = f["id"].to_i(0)
  end
  families
end

def main
  options = {
    family: "0x0",
    output: "flash.uf2"
  }
  parser = OptionParser.new do |opts|
    opts.banner = "Usage: uf2conv.rb [options] INPUT"
    opts.on("-o", "--output FILE", "Write output to named file") { |v| options[:output] = v }
    opts.on("-f", "--family FAMILY", "Specify familyID - number or name") { |v| options[:family] = v }
    opts.on("-c", "--convert", "Do not flash, just convert") # For compatibility, ignored
    opts.on("-b", "--base ADDR", "Set base address (ignored for hex)") # For compatibility
  end
  parser.parse!

  input_file = ARGV[0]
  unless input_file
    puts "Need input file"
    exit 1
  end

  families = load_families
  family_id = 0
  if families.key?(options[:family].upcase)
    family_id = families[options[:family].upcase]
  else
    begin
      family_id = options[:family].to_i(0)
    rescue ArgumentError
      puts "Family ID must be a number or one of: #{families.keys.join(', ')}"
      exit 1
    end
  end

  begin
    hex_content = File.read(input_file)
    uf2_content, start_addr = convert_from_hex_to_uf2(hex_content, family_id)

    File.open(options[:output], 'wb') do |f|
      f.write(uf2_content)
    end
    puts "Converted to uf2, output size: #{uf2_content.length}, start address: 0x#{start_addr.to_s(16)}"
    puts "Wrote #{uf2_content.length} bytes to #{options[:output]}"

  rescue => e
    puts "Error: #{e.message}"
    puts e.backtrace
    exit 1
  end
end

if __FILE__ == $0
  main
end
