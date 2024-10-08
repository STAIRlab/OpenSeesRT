#==============================================================================
# 
#        OpenSees -- Open System For Earthquake Engineering Simulation
#                Pacific Earthquake Engineering Research Center
#
#==============================================================================

target_sources(OPS_Handler
  PRIVATE
    OPS_Stream.cpp
    StandardStream.cpp
    FileStream.cpp
    XmlFileStream.cpp
    DataFileStream.cpp
    DataFileStreamAdd.cpp
    BinaryFileStream.cpp
    DatabaseStream.cpp
    DummyStream.cpp
    TCP_Stream.cpp
    ChannelStream.cpp
  PUBLIC
    OPS_Stream.h
    StandardStream.h
    FileStream.h
    XmlFileStream.h
    DataFileStream.h
    DataFileStreamAdd.h
    BinaryFileStream.h
    DatabaseStream.h
    DummyStream.h
    TCP_Stream.h
    ChannelStream.h
)

target_include_directories(OPS_Handler PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(OPS_Handler PRIVATE OPS_Actor OPS_Logging)

if (WIN32)
  target_link_libraries(OPS_Handler PRIVATE  wsock32 ws2_32)
endif()

