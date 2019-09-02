# cet_script(makeConfigs.rb)

file(GLOB ERBFCL_TEMPLATES "*.erb.fcl")
install( FILES ${ERBFCL_TEMPLATES}
         DESTINATION ${flavorqual_dir}/../snippets )

file(GLOB FCL_TEMPLATES "templates/*.fcl")
install( FILES ${FCL_TEMPLATES}
         DESTINATION ${flavorqual_dir}/../snippets/templates )

# Is this necessary?
#install_source()
