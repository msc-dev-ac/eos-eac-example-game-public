# eos-eac-example-game-public
Minimal game using Epic Online Services (EOS) Easy Anti Cheat (EAC) system. Contains both a server and client component.

## Installation
The EOS SDK is included in the `SDK/` directory.  All keys and IDs relating to the test game setup on the Epic Games Dev Portal are redacted and must be filled in. First create an Epic Games Developer account and then replace all instances of `PRODUCT_ID_HERE`, `SANDBOX_ID_HERE` and `DEPLOYMENT_ID_HERE` with the relevant IDs. ou will also need to download the integrity tool certificates from the Epic Games dashboard in order to build the executables. 

```bash
# build the client and server components
$> cd client && ./build.sh; cd -
$> cd server && ./build.sh; cd -

# run the server
$> cd server && ./run.sh; cd -

# run the client
$> cd client && ./run.sh; cd -

# look out for EAC authentication notification on the server
...
[AUTH STATUS CHANGE] status: 1 # EOS_ACCCAS_LocalAuthComplete
...
[AUTH STATUS CHANGE] status: 2 # EOS_ACCCAS_RemoteAuthComplete

# strace the game
$> sudo strace -ff -p $(pidof MyEOSGame)
```