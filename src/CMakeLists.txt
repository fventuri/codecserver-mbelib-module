add_library(mbelib SHARED mbelibdriver.cpp mbelibdevice.cpp mbelibsession.cpp)
target_link_libraries(mbelib codecserver ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(mbelib PRIVATE ${LIBMBE_INCLUDE_DIR})
target_link_libraries(mbelib ${LIBMBE_LIBRARY})
install(TARGETS mbelib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/codecserver
)
