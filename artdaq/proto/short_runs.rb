#!/usr/bin/env ruby

def get_short_runs(input_file,scale)
    h = {}
    File.open(input_file,"r").each_line do |lin|
	s=lin.split
	h[s[0].to_i] = (s[1].to_f * scale).to_i
    end
    h
end

if __FILE__ == $PROGRAM_NAME

  if ARGV.size < 1
    puts "arguments: short_run_file"
    exit 1
  end

  h=get_short_runs(ARGV[0],1.0)
  p h

end
