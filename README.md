# Make a custom scrubbing animation for ESP32 plays on a TFT
## There are 3 stages from downloading a .gif or .mp4 file and to be able to play it frame by frame (like scrubbing using a potentiometer) on a TFT screen
## Stage 1
### Converting .gif or .mp4 into series of image frames
Use a gif maker tool [Ezgif](https://ezgif.com/video-to-jpg) (recommended) to convert your short video or a .gif file into frames.
Make sure to crop them all inside the settings in **Ezgif** and resize according to your TFT dimensions (for me thats 240x320 px).
Download these frames and they will be used in the python script next.
## Stage 2
### Transforming the frames into C-arrays
The python script uses a very intelligent algorithm to save flash storage and RAM on your microcontroller.
The frames are converted into C-arrays using **RLE (Run Length Encoding)** that saves a huge amount of space for solid backgrounds and large displays. (Note that this won't be effective for complex images with alot of colors and shadow details).
To save some additional bytes inside the flash memory and microcontroller's RAM, the script automatically generates a bounding box for your animation and saves the dimensions as well as the position for this bounding box inside your generated .h file. The arduino code uses these definitions to efficiently write sprite only inside the bounding box.
You can copy the script and use google collab to run it. I did the same!
## Stage 3
### Uploading the code
Now you have the .h file (frames.h as defined in python script). Add this file inside your arduino project folder so you see the following inside your IDE

<img width="346" height="96" alt="image" src="https://github.com/user-attachments/assets/d60dedf0-c94b-4f07-a332-591c85097637" />

Edit the tft. methods to match your use case and hit upload!

I tested this for a 50 frame animation and for that scale, this utilizes a lot less flash memory to save all the frame inside esp32. The RAM is consumed for a single frame only at a time.
