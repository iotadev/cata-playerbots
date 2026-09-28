CollectSourceFiles("${CMAKE_CURRENT_SOURCE_DIR}/src" PLAYERBOT_SOURCES)
CollectIncludeDirectories("${CMAKE_CURRENT_SOURCE_DIR}/src" PLAYERBOT_INCLUDES)
CollectSourceFiles("${CMAKE_CURRENT_SOURCE_DIR}/tests" PLAYERBOT_TEST_SOURCES)
tc_register_module(
  NAME mod-playerbots
  SOURCES ${PLAYERBOT_SOURCES}
  INCLUDE_DIRS ${PLAYERBOT_INCLUDES}
  TEST_SOURCES ${PLAYERBOT_TEST_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Bot/Engine/NamedObjectContext.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Bot/Engine/AiObjectContext.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Bot/Engine/Engine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Bot/Engine/WorldPacket/Event.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Bot/Engine/Action/Action.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Bot/Engine/Trigger/Trigger.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Bot/Engine/Strategy/Strategy.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Script/WorldThr/Queue.cpp"
  TEST_LIBRARIES game-interface
  LOADER AddSC_playerbots_module
  CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/conf/playerbots.conf.dist")
