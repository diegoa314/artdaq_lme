
# 27-Jan_2014, JCF - will be loading in FHiCL code from files located
# in directories referred to by the FHICL_FILE_PATH variable; supply
# only the basename of the file (e.g., read_fcl("WFViewer.fcl"), not
# read_fcl("/my/full/path/WFViewer.fcl")

# The "scrub" option, which defaults to true, will wipe out any lines
# which are only whitespace or which contain comments; this means that
# if you augment a chunk of FHiCL code by reading in a FHiCL file
# where certain parameters get set and there's a set of comments
# describing those parameters, in some cases you can improve
# legibility of the FHiCL code you're augmenting by not including the
# comments, whitespaces, etc.

def read_fcl( filename, scrub = true)
  paths = ENV['FHICL_FILE_PATH'].split(':')

  paths.each do |path|                                                                            

    fullname = path + "/" + filename

    if File.file?( fullname )
      if ! scrub 
        return File.read( fullname )
      else
        text = File.read( fullname )
        text.gsub!(/^\s*\#.*/, "")
        text.gsub!(/^\s*$/, "")
        return text
      end
    end
  end

  raise Exception.new("Unable to locate desired file " + 
                      filename + 
                      " in any of the directories referred to by the " +
                      "FHICL_FILE_PATH environment variable")
end

