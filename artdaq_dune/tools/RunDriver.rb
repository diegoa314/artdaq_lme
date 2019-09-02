#!/usr/bin/env ruby

# JCF, 2-11-14

# This script is meant to provide a very simple example of how one
# might control an artdaq-based program from the command line. The
# idea is that this script uses its command line arguments to generate
# a FHiCL document which in turn is passed to the artdaq-demo's
# "driver" program for processing.

# DemoControl.rb, the main script used to run the full teststand
# simulation, operates off of essentially the same principle -- i.e.,
# use command line arguments to configure a FHiCL script which is in
# turn sent to executables for configuration and processing

# Here, we simply create a FHiCL script designed to run the new
# "ToyDump" module so as to print ADC values from fragments of type
# TOY1 or TOY2 to screen (see the artdaq-demo/Overlays/ToyFragment.hh
# file for more, and/or look at the online Wiki,
# cdcvs.fnal.gov/redmine/projects/artdaq-demo/wiki

require File.join( File.dirname(__FILE__), 'generateToy' )
require File.join( File.dirname(__FILE__), 'generateNormal' )
require File.join( File.dirname(__FILE__), 'generatePattern' )

# A common way to de-facto forward declare functions in Ruby (as is
# the case here with generateToy) is to bundle the main program flow
# into a function (here called "main" in honor of C/C++) and call the
# function at the end of the file

def main

  # More sophisticated programs also use the "OptionParser" class for
  # command line processing

  if ! ARGV.length.between?(2,4)
    puts "Usage: " + __FILE__.split("/")[-1] + " <fragment type = TOY1, TOY2> "  + " <# of events> <generator = Uniform, Normal, Pattern> (nADCcounts) (saveFHiCL)"
    exit 1
  end

  fragtype = ARGV[0]
  nEvents = ARGV[1]
  generator = ARGV[2]

  # We'll pass "nADCcounts" to the generateToy function. If nADCcounts
  # is "nil", generateToy will search for a FHiCL file called
  # "ToySimulator.fcl" for the definition of nADCcounts


  if ARGV.length >= 4
    if (ARGV[3] != "0" && ARGV[3] != "false" && ARGV[3] != "nil")
      saveFHiCL = true
    end
  else
    saveFHiCL = false
  end

  if ARGV.length >= 5
    nADCcounts = ARGV[4]
  else
    nADCcounts = nil
  end


  if fragtype == "TOY1" || fragtype == "TOY2"

    # From generateToy.rb :

    # def generateToy(startingFragmentId, boardId, 
    # fragmentType, throttleUsecs, nADCcounts = nil)
    case generator
    when "Uniform"
        generatorCode = generateToy(0, 0, fragtype, 0, nADCcounts)
    when "Normal"
        generatorCode = generateNormal(0, 0, fragtype, 0, nADCcounts)
    when "Pattern"
        generatorCode = generatePattern(0, 0, fragtype, 0, nADCcounts)
    else
        puts "Invalid Generator Type Supplied!"
        exit
    end
  
  else

    raise "Unknown fragment type \"%s\" passed to the script" % [ fragtype ]
       
  end

  driverCode = generateDriver( generatorCode, nEvents)

  # Backticks can be used to make system calls in Ruby. Here, we
  # create a unique filename, and strip the trailing newline returned
  # by the shell

  filename = `uuidgen`.strip

  # Now dump the driver's FHiCL code to the new file
  
  handle = File.open( filename, "w")
  handle.write( driverCode )
  handle.close

  # And now we call the "driver" program with the temporary FHiCL file

  # Note that since the backticks return stdout, we just stick the
  # call in front of a "puts" command to see the driver's output

  cmd = "demo_driver -c " + filename
  puts `#{cmd}`

  # Either remove or save the FHiCL file
  
  if saveFHiCL
    cmd = "mv " + filename + " renameme.fcl"
  else
    cmd = "rm -f " + filename
  end

  `#{cmd}`

end


def generateDriver(generatorCode, eventsToGenerate)

  driverConfig = String.new( "\

  events_to_generate: %{events_to_generate}
  run_number: 101

  fragment_receiver: {

    %{generator_code}
  }  		  

  event_builder: {

     expected_fragments_per_event: 1
     use_art: true
     print_event_store_stats: false
     verbose: false
     events_expected_in_SimpleQueueReader: @local::events_to_generate
}

######################################################################
# The ART code
######################################################################

physics:
{
  analyzers:
  {
    toyDump:
    {
      module_type: ToyDump
      raw_data_label: daq
      frag_type: @local::fragment_receiver.fragment_type
      num_adcs_to_show: 10   # Obviously this should be no more than ADC counts per fragment
    }
  }

  a1: [ toyDump ]
  #  e1: [ out1 ]
  end_paths: [ a1 ]
}

outputs:
  {
  out1:
  {
    module_type: FileDumperOutput
    wantProductFriendlyClassName: true
  }
}

source:
  {
  module_type: DemoInput
  waiting_time: 900
  resume_after_timeout: true
}

process_name: Driver
" )

 # Only two values are subbed in, although one of them is an entire
 # chunk of FHiCL code used to configure the fragment generator
                      
 driverConfig.gsub!(/\%\{generator_code\}/, generatorCode)
 driverConfig.gsub!(/\%\{events_to_generate\}/, eventsToGenerate)

 return driverConfig
                      
end



main
