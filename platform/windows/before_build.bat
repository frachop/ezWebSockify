C:
CD \
MKDIR C:\projects\ezWebSockify.deps\src
git clone -q --branch=v1.x https://github.com/gabime/spdlog.git C:\projects\ezWebSockify.deps\src\spdlog\
CD C:\projects\ezWebSockify.deps\src\spdlog
git checkout -qf 616caa5d30172b65cc3a06800894c575d70cb8e6
MKDIR ..\..\include
MOVE include\spdlog ..\..\include\
