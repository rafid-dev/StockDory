include("/media/rafid/Resources/Projects/StockDory/RB/cmake/CPM_0.38.0.cmake")
CPMAddPackage("NAME;thread-pool;GITHUB_REPOSITORY;DeveloperPaul123/thread-pool;GIT_TAG;0.5.1;OPTIONS;TP_BUILD_TESTS OFF;TP_BUILD_BENCHMARKS OFF;TP_BUILD_EXAMPLES OFF")
set(thread-pool_FOUND TRUE)