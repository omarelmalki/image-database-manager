# image-database-manager

Image database manager, inspired by Facebook's Haystack, made for social media websites to improve performance with images
- Stores images in three resolutions (thumbnail, small, and original resolution) to optimize the time needed to view an image in a smaller/bigger resolution
- Avoids storing duplicates with SHA-256

Libraries needed:
- libssl (Ubuntu: sudo apt install libssl-dev libssl-doc, MacOS: brew install openssl)
- libvips (Ubuntu: sudo apt-get install libvips-dev, MacOS: brew install vips)
- libmongoose (source code already included)
- libjson (Ubuntu: sudo apt install libjson-c-dev, MacOS: brew install json-c)

How to use:
- Clone/download the repo on your machine
- Compile and link the project by typing "make" in the command line
- Type "./imgStoreMgr help" in the command line to display the different commands available

Example list of instructions:
- Create a new file: "./imgStoreMgr create test_file"
- Add an image named "pineapple.jpg" located in the same folder as imgStoreMgr: "./imgStoreMgr insert test_image pineapple.jpg"

How to view and edit a file visually on a localhost server:
- If not created, create a new file: "./imgStoreMgr create test_file"
- export the LD_LIBRARY_PATH pointing to libmongoose: "export LD_LIBRARY_PATH="${PWD}"/libmongoose" or "export DYLD_FALLBACK_LIBRARY_PATH="${PWD}"/libmongoose"
- Start server: "./imgStore_server test_file"
- Open server by going to http://localhost:8000
- The server was made for testing purposes allowing the developer to insert, delete, list images and view them in different resolutions

Emilien Duc and Omar El Malki, EPFL, Switzerland, 2021
