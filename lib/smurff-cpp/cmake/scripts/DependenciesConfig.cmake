macro(configure_pthreads)
    message ("Dependency check for pthreads multi-threading support...")

    if(UNIX)
        find_package(Threads REQUIRED)
        if(Threads_FOUND)
            message(STATUS "Found threading library")
            if(CMAKE_USE_PTHREADS_INIT)
                message(STATUS "Found pthreads " ${CMAKE_THREAD_LIBS_INIT})
            else()
                message(STATUS "Pthreads not found")
            endif()
        else()
            message(STATUS "Threading library not found")
        endif()
    else()
       message(STATUS "Not required on windows")
    endif()
endmacro(configure_pthreads)

macro(configure_mpi)
  message ("Dependency check for mpi...")

  find_package(MPI)
  if(${MPI_C_FOUND})
    message(STATUS "MPI found")
  else()
    message(STATUS "MPI not found")
  endif()
   
endmacro(configure_mpi)

macro(configure_openmp)
  message ("Dependency check for OpenMP")

  find_package(OpenMP)
  if(${OPENMP_FOUND})
      message(STATUS "OpenMP found")
      set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OpenMP_CXX_FLAGS}")
      set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${OpenMP_C_FLAGS}")
      message(STATUS "OpenMP_CXX_LIB_NAMES ${OpenMP_CXX_LIB_NAMES}")
      message(STATUS "OpenMP_CXX_LIBRARY ${OpenMP_CXX_LIBRARY}")
      message(STATUS "OpenMP_CXX_LIBRARIES ${OpenMP_CXX_LIBRARIES}")
  else()
      message(STATUS "OpenMP not found")
  endif()
   
endmacro(configure_openmp)

macro(configure_lapack)
  message ("Dependency check for lapack...")
  find_package(LAPACK REQUIRED)
  add_definitions(-DEIGEN_USE_BLAS -DEIGEN_USE_LAPACKEyy)
  message(STATUS LAPACK: ${LAPACK_LIBRARIES})
endmacro(configure_lapack)

macro(configure_openblas)
  message ("Dependency check for openblas...")
  
  if(MSVC)
  set(BLAS_LIBRARIES  $ENV{BLAS_LIBRARIES})
  set(BLAS_INCLUDES $ENV{BLAS_INCLUDES})
  set(BLAS_FOUND ON)
  else()
  set(BLA_VENDOR "OpenBLAS")
  find_package( BLAS REQUIRED )
  endif()

  add_definitions(-DEIGEN_USE_BLAS -DEIGEN_USE_LAPACKEyy)

  message(STATUS BLAS: ${BLAS_LIBRARIES} )
 
endmacro(configure_openblas)

macro(configure_mkl)
  message ("Dependency check for MKL (using MKL SDL)...")
  find_library (MKL_LIBRARIES "mkl_rt" HINTS ENV LD_LIBRARY_PATH REQUIRED)
  find_PATH (MKL_INCLUDE_DIR "mkl.h" HINTS ENV CPATH REQUIRED)
  include_directories(${MKL_INCLUDE_DIR})

  # since we mix OpenMP and mkl we need to link this
  if(${OPENMP_FOUND})
      add_definitions(-DMKL_THREAD_LIBRARY )
      list(FIND OpenMP_CXX_LIB_NAMES "gomp" GNU_OPENMP)
      list(FIND OpenMP_CXX_LIB_NAMES "iomp5" INTEL_OPENMP)
      list(FIND OpenMP_CXX_LIB_NAMES "omp" LLVM_OPENMP)
      if(NOT GNU_OPENMP EQUAL -1)
          add_definitions(-DMKL_THREAD_LIBRARY_GNU )
      elseif(NOT INTEL_OPENMP EQUAL -1)
          add_definitions(-DMKL_THREAD_LIBRARY_INTEL )
      elseif(NOT LLVM_OPENMP EQUAL -1)
          add_definitions(-DMKL_THREAD_LIBRARY_LLVM )
      else()
          message(ERROR "Unknown threading library ${OpenMP_CXX_LIB_NAMES}")
      endif()
  else()
      add_definitions(-DMKL_THREAD_LIBRARY_SEQUENTIAL )
  endif()
  
  add_definitions(-DEIGEN_USE_MKL_ALL)
  
  message(STATUS MKL: ${MKL_LIBRARIES} )
endmacro(configure_mkl)

macro(configure_eigen)
  message ("Dependency check for eigen...")
  
  if(DEFINED ENV{EIGEN3_INCLUDE_DIR})
  SET(EIGEN3_INCLUDE_DIR $ENV{EIGEN3_INCLUDE_DIR})
  else()
  find_package(Eigen3 REQUIRED)
  endif()

  
  message(STATUS EIGEN3: ${EIGEN3_INCLUDE_DIR})
endmacro(configure_eigen)

macro(configure_boost)
  if(${ENABLE_BOOST})
      message ("Dependency check for boost...")
      
      set (Boost_USE_STATIC_LIBS OFF)
      set (Boost_USE_MULTITHREADED ON)

      # find boost random library - optional

      if(${ENABLE_BOOST_RANDOM})
        set (BOOST_COMPONENTS random)

        FIND_PACKAGE(Boost COMPONENTS ${BOOST_COMPONENTS})

        if(Boost_FOUND)
            message("Found Boost random library")
            message("Found Boost_VERSION: ${Boost_VERSION}")
            message("Found Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
            message("Found Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")

            set(BOOST_RANDOM_LIBRARIES ${Boost_LIBRARIES})
            set(BOOST_RANDOM_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})

            add_definitions(-DUSE_BOOST_RANDOM)
            message("Boost minor: ${Boost_MINOR_VERSION}")
            if ((${Boost_MINOR_VERSION} LESS "60") AND (${Boost_MINOR_VERSION} GREATER_EQUAL "54"))
                add_definitions(-DTEST_RANDOM)
                message("Enabling test cases that depend on Boost random 1.5x.y")
            endif()
        else()
            message("Boost random library is not found")
        endif()
      endif()

      #find boost program_options library - required

      set (BOOST_COMPONENTS system 
                            program_options)

      FIND_PACKAGE(Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
      
      #see https://stackoverflow.com/questions/28887680/linking-boost-library-with-boost-use-static-lib-off-on-windows
      if (MSVC)
         # disable autolinking in boost
         add_definitions(-DBOOST_ALL_NO_LIB)

         # force all boost libraries to dynamic link (we already disabled
         # autolinking, so I don't know why we need this, but we do!)
         add_definitions(-DBOOST_ALL_DYN_LINK)
      endif()
  endif()

  if(Boost_FOUND)
      message("Found Boost_VERSION: ${Boost_VERSION}")
      message("Found Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
      message("Found Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
      add_definitions(-DHAVE_BOOST)
  else()
      message("Boost library is not found")
  endif()
endmacro(configure_boost)

macro(configure_python)
    if(ENABLE_PYTHON)
        find_package(PythonInterp REQUIRED)
        find_package(NumPy REQUIRED)
        find_package(PythonLibs REQUIRED)
        find_package(PythonExtensions REQUIRED)
        find_package(Cython REQUIRED)
    endif()
endmacro(configure_python)
