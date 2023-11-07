# GPNC (General Purpose Network-graphic Controller)
Image modifier made with C and GTK+

## How to Install
```
Use these commands in your terminal.

For mac and linux only:
 - git clone https://github.com/MelonFruit7/GPNC.git
 - cd GPNC

  If you are on linux
    - sudo apt install libgtk-3-dev
  If you are on mac
    If you don't have brew
      - /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    - brew install gtk+3
    - brew install pkg-config

  - make
  - ./GPNC


After this you are all good 
```
---

| Main Features | Description |
| ------------- | ----------- |
| &Choose Image | Allows a user to select an image from their machine |
| &Resize Button | Resizes the image to fit your screen (will not resize for saving purposes) | 
| Color Button | Click on a pixel on your image and the color will show up |
| &Zooming | Zoom in/out of the image by moving your scroll wheel, will apply directly on the pixel you hover over |
| Panning | Move your mouse while holding left click to pan through the image when zoomed in |
| &Crop Button | Will crop the image exactly as you see it (zoom into the area you want to crop and click crop) |
| Keep Aspect Ratio | As the name implies when selecting/resizing your image the aspect ratio will be maintined |
| Modify Button | Opens the modifier functions menu |
| Save | Save the image to your computer (**WILL NOT WARN YOU IF FILE HAS SAME NAME**) |

<br><br><br>

| Modifier Functions | Description |
| ------------------ | ----------- |
| Darken* | Darken the image (Deafult entry: 2) |
| Greyscale | Greyscale the image |
| Invert | Invert colors of the image |
| Lighten* | Lighten the image (Default entry: 2) |
| Swirl* | Swirls the image (Default entry: 1) |
| &Flip | Flips the image vertically |
| &Flop | Flips the image horizontally |
| &Rotate Right | Rotates the image towards the right |
| &Rotate Left | Rotates the image towards the left |
| Revert | Undo to the previous version of your image |
| Redo | Bring back the previous image you reverted |
| ZoomLevel* | Change how fast the zoom feature is (Default entry: 30) |
| &ResizeX | Resize the image for saving purposes (resizes the original image) |


| Modifier Suffix | Description |
| ------------------------ | ----------- |
| * | Input a number into the entry box to enhance its effects (usual range 1 - 1000). |
| X | Input a dimension into the entry box such as 200x200 |



#### Quick Selection Feature
- If you hold right click and drag across the image then quick selection will apply in the form of a white line.
- The line will automatically bound to its starting position when you release the right click.
- **Most features will stop working while quick selection is on**.
- You can draw as many bounds as you want (the more bounds you draw the more likely the result will be unstable).
- **To leave quick selection hit the escape key**.
- Quick selection will automatically turn off after you close the modification menu.
- **Side Note: I put a "&" in these docs at the front of features that won't work while quick selection is on.**

---

### Known Issues
#### Shift Quick Selection Error (P2)
> Use a shift modifier and revert (UNDO) to the previous image.
> 
> Now if you use quick selection, open modifier buttons, and immediately click redo then it can cause issues with the program.
>
> Reason: The quick selection bitmap will not be resized to the same dimensions as the image so it has the ability to cause errors.
#### Original File Error (P3)
> Choose an image file.
> 
> Move image file to different location on your computer.
> 
> If you revert (UNDO) the furthest you can then it will cause undefined behavior.
>
> Reason: For the very first save the image isn't saved in the SaveContent folder but the path is stored within the program.
#### SaveContent not cleared (P3)
> Force quit your program after doing some modifications
>
> SaveContent Folder won't be cleared
>
> Reason: I'm actually not sure. I'm assuming it just will not execute the code to clear that folder if you force quit.
