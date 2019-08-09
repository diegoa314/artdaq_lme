#!/usr/bin/env ruby

def cat(input_list, output_name)
  File.open(output_name,"w") do |f|
    input_list.each do |name|
      File.open(name,"r").each_line { |lin| f.puts(lin) }
    end
  end
end

def catb(input_list, output_name)
  File.open(output_name,"wb") do |f|
    input_list.each do |name|
      File.open(name,"rb") do |fr|
        while str = fr.read(2048)
	  f.write(str)
        end
      end
    end
  end
end

if __FILE__ == $PROGRAM_NAME

  if ARGV.size < 2
    puts "arguments: outfile infile1 infile2 ... infileN"
    exit 1
  end

  catb(ARGV[1..-1],ARGV[0])

end
