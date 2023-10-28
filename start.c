#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h> //Let's us use mkdir
#define SAVE_CONTENT_PATH "SaveContent/image%d.jpeg" //This is used in the functions that work with redoing/undoing the image

typedef unsigned char uc;
typedef struct { //This struct is used so we can successfully pass in parameters into the editAllPixels function via a g_signal_connect
  uc* (*operation)(); //If I don't specify params then I can put in as many as I need (idk why this works tbh)
  int factor;
  int params;
} EditArgs;

GdkPixbuf *originalImagePix = NULL; //Image with original aspect ratio
GdkPixbuf *quickSelectBitMap = NULL; //Bit map for quick selection
GdkPixbuf *currentPix = NULL; //Image placeholder
char *orignalImagePath = NULL; //Stores the path of the original image so we can revert if requested by the user
int currentImageWidth, currentImageHeight; //Current image width and height (resized)

GtkWidget *image; //The image container
GtkWidget *keepAspectRatio; //Check Button which maintains aspect ratio if selected
int scrollWidth = 725, scrollHeight = 725; //The scrolled window widget width and height 
int zoomLevel = 40; //This is the amount of pixels we zoom in by the y when scrolling (By default)

int modificationCounter = 0; //Tracks the amount of modifications made to the original image
int maxRedo = 0; //Signifies the max image you can redo to, if it's 0 you are unable to redo

GtkWidget *colorBtn; //Color Button
GtkWidget *entry; //Entry box where users can type scale factors when modifying an image (This widget is in the modify dialog)

bool movingImage = false; //Are we currently moving through our image?
bool quickSelecting = false, quickSelectionOn = false; //Are we currently quick selecting something?
uc quickSelectionColor = 255;
int xMove, yMove; //These are initially set in "onImageClick()", they represent the previous position of the mouse while dragging through the image
int prevXMove, prevYMove; //These store the previous position of our mouse as captured in the "onImageMove" function (They are currently only in use for quick selection)


void windowResized(GtkWidget *widget, GdkRectangle *allocation, gpointer data); //Call when the window is resized
bool keyPress(GtkWidget *widget, GdkEventKey *event, gpointer data); //Detects key press
void reDrawImage(); //Redraw the image
void resizeImage(GtkWidget *widget, gpointer data); //Call when the window is resized
void chooseFile(GtkWidget *widget, gpointer data); //Function to choose a file
void saveFile(GtkWidget *widget, gpointer data); //Function to save a file
void defineCSS(GtkWidget *widget, GtkCssProvider *cssProvider, char* class); //Function used to define css for widgets
bool scrolledResize(GtkWidget *widget, GdkEventScroll *event, gpointer data); //Used to resize the image on scroll
bool onImageClick(GtkWidget *widget, GdkEventButton *event, gpointer data); //Called when you click the image
bool onImageRelease(GtkWidget *widget, GdkEventButton *event, gpointer data); //Called when you release the image
bool onImageMove(GtkWidget *widget, GdkEventMotion *event, gpointer data); //Called while you are dragging the image
void drawQuickSelect(int x, int y); //Quick Selection drawing helper
void connectPixelQuickSelect(uc *pixels, int x1, int y1, int x2, int y2, int rowstride, int channels, uc* pixelConnect); //connect 2 pixels (not a very good method might change later)
GdkPixbuf* applyBitmap(GdkPixbuf *apply); //Apply a bitmap to an image and return a resulting image
void modifyImageDialog(GtkWidget *widget, gpointer data); //Called when user wants to modify the image
void modifyButtonCreate(GtkWidget *grid, int gridPos[4], void (*func)(GtkWidget*, gpointer), gpointer data, char* name); //Creates buttons for the modifyImageDialog
void cropImage(GtkWidget *widget, gpointer data); //Crop the image

//Modify image functions
void editAllPixels(GtkWidget *widget, gpointer data);
uc* greyscalePixel(uc* rgb);
uc* invertPixel(uc* rgb);
uc* darkenPixel(uc* rgb, int factor);
uc* lightenPixel(uc* rgb, int factor);
void flip(GtkWidget *widget, gpointer data);
void flop(GtkWidget *widget, gpointer data);
void rotateRight(GtkWidget *widget, gpointer data);
void rotateLeft(GtkWidget *widget, gpointer data);
uc* swirlImage(uc* rgb, int factor, int x, int y);
void editPixel(uc *pixelsNew, uc* pixelsOld, int offsetNew, int offsetOld, int channels);
void revertImage(GtkWidget *widget, gpointer data);
void redoImage(GtkWidget *widget, gpointer data);
void trackModification();
void resizeSpecific(GtkWidget *widget, gpointer data);
void changeZoomLevel(GtkWidget *widget, gpointer data);


void activate(GtkApplication *app, gpointer data) { //----------------------------------------------------------------------------------------------------------------------------------
  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_default_size(GTK_WINDOW(window), scrollWidth, scrollHeight);
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);
  gtk_window_set_title(GTK_WINDOW(window), "GPNC (General Purpose Network-graphic Controller)");

  //CSS provider (lets us now where the css file is)
  GtkCssProvider *cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(cssProvider, "styles.css", NULL);


  //Vertical box to store the image and the nav bar
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(window), vbox);
  //Horizontal box (nav bar)
  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  //Resize Button
  GtkWidget *resizeBtn = gtk_button_new_with_label("Resize");
  gtk_box_pack_start(GTK_BOX(hbox), resizeBtn, FALSE, FALSE, 0);
  //Color Button
  colorBtn = gtk_color_button_new();
  gtk_box_pack_start(GTK_BOX(hbox), colorBtn, FALSE, FALSE, 0);
  //File chooser
  GtkWidget *fileChooser = gtk_button_new_with_label("Choose Image");
  gtk_box_pack_start(GTK_BOX(hbox), fileChooser, FALSE, FALSE, 0);
  //Image styles
  GtkWidget *modifyBtn = gtk_button_new_with_label("Modify");
  gtk_box_pack_start(GTK_BOX(hbox), modifyBtn, FALSE, FALSE, 0);
  //Crop Button
  GtkWidget *cropBtn = gtk_button_new_with_label("Crop");
  gtk_box_pack_start(GTK_BOX(hbox), cropBtn, FALSE, FALSE, 0);
  //Aspect Ratio Check
  keepAspectRatio = gtk_check_button_new_with_label("Keep Aspect Ratio");
  gtk_box_pack_start(GTK_BOX(hbox), keepAspectRatio, FALSE, FALSE, 0);
  //Save Button
  GtkWidget *saveBtn = gtk_button_new_with_label("Save");
  gtk_box_pack_start(GTK_BOX(hbox), saveBtn, FALSE, FALSE, 0);

  //Create the scroll window
  GtkWidget *scrollBox = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollBox), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(vbox), scrollBox, TRUE, TRUE, 0);

  GtkWidget *eventBox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(scrollBox), eventBox);
  //Create a new image window and put it in the scrollBox
  image = gtk_image_new();
  gtk_container_add(GTK_CONTAINER(eventBox), image);

  //Signal events
  g_signal_connect(window, "key-press-event", G_CALLBACK(keyPress), NULL);
  g_signal_connect(fileChooser, "clicked", G_CALLBACK(chooseFile), NULL);
  g_signal_connect(saveBtn, "clicked", G_CALLBACK(saveFile), NULL);
  g_signal_connect(resizeBtn, "clicked", G_CALLBACK(resizeImage), NULL);
  g_signal_connect(window, "size-allocate", G_CALLBACK(windowResized), scrollBox);
  g_signal_connect(scrollBox, "scroll-event", G_CALLBACK(scrolledResize), NULL);
  g_signal_connect(eventBox, "button-press-event", G_CALLBACK(onImageClick), NULL);
  g_signal_connect(eventBox, "button-release-event", G_CALLBACK(onImageRelease), NULL);
  g_signal_connect(eventBox, "motion-notify-event", G_CALLBACK(onImageMove), scrollBox);
  g_signal_connect(modifyBtn, "clicked", G_CALLBACK(modifyImageDialog), NULL);
  g_signal_connect(cropBtn, "clicked", G_CALLBACK(cropImage), scrollBox);
  


  
  //Define CSS of widgets
  defineCSS(window, cssProvider, "windowView");
  defineCSS(scrollBox, cssProvider, "scrollBox");
  defineCSS(image, cssProvider, "imageView");
  defineCSS(vbox, cssProvider, "vboxContainer");
  defineCSS(hbox, cssProvider, "hboxTopContainer");

  gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
  GtkApplication *app = gtk_application_new("colorSwap.in", G_APPLICATION_NON_UNIQUE);
  
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int ret = g_application_run(G_APPLICATION(app), argc, argv);
  
  g_object_unref(app); 
  char filePath[30];
  for (int i = 1; i <= maxRedo || i <= modificationCounter; i++) { //Clear the saved content since we closed the app
    sprintf(filePath, SAVE_CONTENT_PATH, i);
    remove(filePath);
  } 
  return ret;
}


void defineCSS(GtkWidget *widget, GtkCssProvider *cssProvider, char* class) { //Used to define css for a widget
  GtkStyleContext *context = gtk_widget_get_style_context(widget);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  gtk_style_context_add_class(context, class);
} //FUNCTION END

bool keyPress(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->keyval == GDK_KEY_Escape && !quickSelecting && quickSelectionOn) { //Click escape to get out of quick selection
        //Clear and turn everything off relating to quick selection, then redraw the image
        if (quickSelectBitMap != NULL) g_object_unref(quickSelectBitMap);
        quickSelectBitMap = NULL;
        quickSelectionOn = false;
        reDrawImage();
    }
    return FALSE;
}

void chooseFile(GtkWidget *widget, gpointer data) { //Called when the choose file button is clicked
  if (quickSelectionOn) return;

  GtkFileChooser *chooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(gtk_widget_get_toplevel(image)),GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL,"_Open", GTK_RESPONSE_ACCEPT, NULL));
  int res = gtk_dialog_run(GTK_DIALOG(chooser));
  if (res == GTK_RESPONSE_ACCEPT) {

    char filePath[30];
    for (int i = 1; i <= maxRedo || i <= modificationCounter; i++) { //Clear the saved content from the previous image
      sprintf(filePath, SAVE_CONTENT_PATH, i); //SAVE_CONTENT_PATH is a #define
      remove(filePath);
    } 
    modificationCounter = 0, maxRedo = 0; //Reset the modification counter and maxRedo


    //If the originalImagePix exists unref it and the originalPath since we reset it here
    if (originalImagePix != NULL) {
       g_object_unref(originalImagePix);
       g_free(orignalImagePath);
    }

    //Grab the file path that was given by the file chooser dialog
    orignalImagePath = gtk_file_chooser_get_filename(chooser);

    //Use the file path to load and image
    originalImagePix = gdk_pixbuf_new_from_file(orignalImagePath, NULL);

    //If it wasn't an image then break the program lol
    if (originalImagePix == NULL) exit(1);
    mkdir("SaveContent", 0777); //Creates a new directory called SaveContent so we can use undo/redo

    resizeImage(widget, NULL); //Resize image to fit scroll box
  }

  gtk_widget_destroy(GTK_WIDGET(chooser));
} //FUNCTION END

void saveFile(GtkWidget *widget, gpointer data) {
    if (originalImagePix == NULL) return;
    
    //Open file chooser dialog
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("Save Image", GTK_WINDOW(gtk_widget_get_toplevel(image)),GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL,"_Save", GTK_RESPONSE_ACCEPT, NULL));

    gtk_file_chooser_set_current_name(chooser, "image.png");
    int res = gtk_dialog_run(GTK_DIALOG(chooser));
    if (res == GTK_RESPONSE_ACCEPT) { //If the user clicked save
      char *filePath = gtk_file_chooser_get_filename(chooser);
      gdk_pixbuf_save(originalImagePix, filePath, "png", NULL, NULL); //Save the image at the filepath in png format
      g_free(filePath);
    }
    gtk_widget_destroy(GTK_WIDGET(chooser));
}


void reDrawImage() { //Will redraw the pixbuf onto the image widget
  GdkPixbuf *temp = gdk_pixbuf_scale_simple(originalImagePix, currentImageWidth, currentImageHeight, GDK_INTERP_NEAREST);
  gtk_image_set_from_pixbuf(GTK_IMAGE(image), temp);
  g_object_unref(temp);
} //FUNCTION END
void resizeImage(GtkWidget *widget, gpointer data) { //Called when the resize button is clicked
  if (originalImagePix == NULL || quickSelectionOn) return;
  
  //Here we make sure if we are keeping the aspect ratio we also make the image larger than the scollBox
  bool keepRatio = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(keepAspectRatio));

  //Get the current width and height of the image and get the aspect ratio
  int width = gdk_pixbuf_get_width(originalImagePix), height = gdk_pixbuf_get_height(originalImagePix);
  double aspectR = (double)width/height;
  double sAspectR = (double)scrollWidth/scrollHeight;

  //If the aspect ratio (w/h) of the image is greater than the aspect ratio (w/h) of the screen, it means we have to set the image in relation to the height rather than the width to make it take up the entire screen.
  width = scrollWidth, height = (int)(width/aspectR); //width after the , is actually now scrollWidth
  if (aspectR > sAspectR) height = scrollHeight, width = (int)(height*aspectR); //height after the , is actually now scrollHeight

   //If we don't keep the aspect ratio just make it the same size as the scroll window
  if (!keepRatio) {width = scrollWidth, height = scrollHeight;}
  
  currentPix = gdk_pixbuf_scale_simple(originalImagePix, width, height, GDK_INTERP_NEAREST); //Resize the pixbuf image
  gtk_image_set_from_pixbuf(GTK_IMAGE(image), currentPix);  //Set the pixbuf to the image view
 
  currentImageWidth = width;
  currentImageHeight = height;

  g_object_unref(currentPix);
} //FUNCTION END


void windowResized(GtkWidget *widget, GdkRectangle *allocation, gpointer data) { //Called when you resize the window
  GtkWidget *scroll = GTK_WIDGET(data);
  
  scrollWidth = gtk_widget_get_allocated_width(scroll);
  scrollHeight = gtk_widget_get_allocated_height(scroll);
} //FUNCTION END

bool scrolledResize(GtkWidget *widget, GdkEventScroll *event, gpointer data) { //Called when you scroll on the image
  if (quickSelectionOn) return 1; //No zooming while quick selecting

  //Sometimes delta_y starts at 0 for no reason and it causes weird behavior in the program ;-;
  if (originalImagePix == NULL || event->delta_y == 0) return 1;

  bool zoomIn = event->delta_y <= 0; //This checks if we are scrolling in or out
  if ((currentImageHeight > 3000 || currentImageWidth > 3000) && zoomIn) {return 1;} else if((currentImageHeight <= zoomLevel || currentImageWidth <= zoomLevel) && !zoomIn) {return 1;}

  double aspectR = (double)currentImageWidth/currentImageHeight;
  int add = zoomIn ? zoomLevel : -zoomLevel; //This is the zoomIn add on the height

  //Scale the image in relation to its aspect ratio
  currentPix = gdk_pixbuf_scale_simple(originalImagePix, currentImageWidth+(int)(add*aspectR), currentImageHeight+add, GDK_INTERP_NEAREST);
  gtk_image_set_from_pixbuf(GTK_IMAGE(image), currentPix);

  //FInd out where the scroll bars are
  GtkAdjustment *vScroll = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(widget));
  GtkAdjustment *hScroll = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(widget));
  
  //event->y and event->x are the mouses coordinates respectively vertical and horizontal
  /*------------------------------------------------------------------------------------
    Imagine our mouse was the at the top left corner of the image and we were zooming in.
    The top left corner is (0,0) for x and y.
    If we were zooming into the top left corner we wouldn't want to adjust the scroll wheel at all.
    When the image gets larger the scroll wheels don't change position. However, the image gets larger and it appears as though the scroll wheels went up and left.
    So when we are adjusting we actually have to account for where the mouse is, if you imagine the opposite (bottom right corner) we want to adjust the maximum amount to keep up.

    This works fine, but it isn't perfect. (Fixed version: uses the same logic but uses the position of the mouse on the image instead of the scroll box)
    ------------------------------------------------------------------------------------
  */
  double displacementV = add*((event->y+gtk_adjustment_get_value(vScroll))/currentImageHeight), displacementH = ((int)(add*aspectR))*((event->x+gtk_adjustment_get_value(hScroll))/currentImageWidth);


  //This is done automatically after the function ends, but we set it here in the case the adjustment value needs to be there
  gtk_adjustment_set_upper(hScroll,  gtk_adjustment_get_upper(hScroll)+((int)(add*aspectR)));
  gtk_adjustment_set_upper(vScroll,  gtk_adjustment_get_upper(vScroll)+ add);


  gtk_adjustment_set_value(hScroll, gtk_adjustment_get_value(hScroll)+displacementH);
  gtk_adjustment_set_value(vScroll, gtk_adjustment_get_value(vScroll)+displacementV);


  g_object_unref(currentPix);

  currentImageWidth += (int)(add*aspectR);
  currentImageHeight += add;

  return 1;
} //FUNCTION END

bool onImageClick(GtkWidget *widget, GdkEventButton *event, gpointer data) { //Called when you click on the image
  int startPosW = currentImageWidth < scrollWidth ? (scrollWidth-currentImageWidth)/2 : 0; //The x position where the image starts appearing on the display
  int startPosH = currentImageHeight < scrollHeight ? (scrollHeight-currentImageHeight)/2 : 0; //The y position where the image starts appearing on the display
  if (event->x <= startPosW || event->x >= startPosW+currentImageWidth || event->y <= startPosH || event->y >= startPosH+currentImageHeight) return 1; //Didn't click on the image

  if (event->button == GDK_BUTTON_PRIMARY && !quickSelecting) { //User wants to pan through the image
    xMove = event->x;
    yMove = event->y;
    
    movingImage = true;

    if (originalImagePix != NULL) {
      currentPix = gdk_pixbuf_scale_simple(originalImagePix, currentImageWidth, currentImageHeight, GDK_INTERP_NEAREST);
      uc *pixels = gdk_pixbuf_get_pixels(currentPix);
      int rowstride = gdk_pixbuf_get_rowstride(currentPix), channels = gdk_pixbuf_get_n_channels(currentPix);

      GdkRGBA color;
      int offset = (yMove-startPosH)*rowstride + (xMove-startPosW)*channels; //Find the offset to get to the pixel we clicked on
      color.red = pixels[offset]/255.0;
      color.green = pixels[offset+1]/255.0;
      color.blue = pixels[offset+2]/255.0;
      color.alpha = 1.0;
      gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(colorBtn), &color); //Set the color of the color button to the color of the pixel we clicked on
      g_object_unref(currentPix);
    }
  } else if (event->button == GDK_BUTTON_SECONDARY && !movingImage) { //User wants to use quick selection tool
    xMove = event->x;
    yMove = event->y;
    quickSelecting = true;
    quickSelectionOn = true;

    if (quickSelectBitMap == NULL) { //Create a new quickSelectBitMap only if one doesn't currently exist
      quickSelectBitMap = gdk_pixbuf_new(GDK_COLORSPACE_RGB, (gdk_pixbuf_get_n_channels(originalImagePix) == 4 ? TRUE : FALSE), 8, currentImageWidth, currentImageHeight);
      int rowstride = gdk_pixbuf_get_rowstride(quickSelectBitMap), channels = gdk_pixbuf_get_n_channels(quickSelectBitMap);

      uc *quickPixels = gdk_pixbuf_get_pixels(quickSelectBitMap);
      uc *blackPixels = malloc(4*sizeof(uc));
      for (int i = 0; i < 4; i++) blackPixels[i] = 0;

      for (int i = 0; i < currentImageHeight; i++) { //Initialize all pixels in the bitmap to black
        for (int j = 0; j < currentImageWidth; j++) {
          int offset = i*rowstride + j*channels;
          editPixel(quickPixels, blackPixels, offset, 0, channels);
        }       
      }
      free(blackPixels);
    }

    
    GdkPixbuf *temp = gdk_pixbuf_scale_simple(originalImagePix, currentImageWidth, currentImageHeight, GDK_INTERP_NEAREST);
    currentPix = applyBitmap(temp);
    g_object_unref(temp);


    //Set our previous moves (our moves here are now our previous moves)
    prevXMove = xMove; 
    prevYMove = yMove;
    drawQuickSelect(xMove, yMove);
  }
  return 1;
} //FUNCTION END

bool onImageMove(GtkWidget *widget, GdkEventMotion *event, gpointer data) { //Captures Mouse movement through the image
  if (movingImage) { //movingImage is true if we are holding down on our mouse
    GtkAdjustment *hAdj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(data));
    GtkAdjustment *vAdj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(data));

    int adjX = xMove - event->x;
    int adjY = yMove - event->y;

    //Moves scroll wheel based on where our original mouse position was
    gtk_adjustment_set_value(hAdj, gtk_adjustment_get_value(hAdj)+adjX); 
    gtk_adjustment_set_value(vAdj, gtk_adjustment_get_value(vAdj)+adjY);
  } else if (quickSelecting) {
    drawQuickSelect((int)event->x, (int)event->y);
  } 
  return 1;
} //FUNCTION END

bool onImageRelease(GtkWidget *widget, GdkEventButton *event, gpointer data) { //Called after you release a click on the image
  if (event->button == GDK_BUTTON_PRIMARY && movingImage) movingImage = false;
  if (event->button == GDK_BUTTON_SECONDARY && quickSelecting) {
     quickSelecting = false;
     drawQuickSelect(xMove, yMove);
     g_object_unref(currentPix);
  }
  return 1;
} //FUNCTION END

void drawQuickSelect(int x, int y) { //Drawing helper for quick selection (x and y should not be adjusted for bounds, it will be done in this function)
  int startPosW = currentImageWidth < scrollWidth ? (scrollWidth-currentImageWidth)/2 : 0; //The x position where the image starts appearing on the display
    int startPosH = currentImageHeight < scrollHeight ? (scrollHeight-currentImageHeight)/2 : 0; //The y position where the image starts appearing on the display
    if (x <= startPosW || x >= startPosW+currentImageWidth || y <= startPosH || y >= startPosH+currentImageHeight) return; //No longer on the image

    uc* pixels = gdk_pixbuf_get_pixels(currentPix);
    int rowstride = gdk_pixbuf_get_rowstride(currentPix), channels = gdk_pixbuf_get_n_channels(currentPix);
    uc* colorPixels = malloc(4*sizeof(uc));
    for (int i = 0; i < 4; i++) colorPixels[i] = quickSelectionColor;

    editPixel(pixels, colorPixels, ((int)y - startPosH)*rowstride + ((int)x - startPosW)*channels, 0, channels); //Paint a single pixel
    //Connect current recorded mouse position and previous recorded mouse position with pixels to fill in gaps
    connectPixelQuickSelect(pixels, prevXMove-startPosW, prevYMove-startPosH, ((int)x- startPosW), ((int)y - startPosH), rowstride, channels, colorPixels); 


    pixels = gdk_pixbuf_get_pixels(quickSelectBitMap); //Change pixels to the one on the bitmap
    editPixel(pixels, colorPixels, ((int)y - startPosH)*rowstride + ((int)x - startPosW)*channels, 0, channels);
    //Connect current recorded mouse position and previous recorded mouse position with pixels to fill in gaps on the bitmap
    connectPixelQuickSelect(pixels, prevXMove-startPosW, prevYMove-startPosH, ((int)x- startPosW), ((int)y - startPosH), rowstride, channels, colorPixels);

    free(colorPixels);
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), currentPix);

    prevXMove = (int)x;
    prevYMove = (int)y;
} //FUNCTION END

void connectPixelQuickSelect(uc *pixels, int x1, int y1, int x2, int y2, int rowstride, int channels, uc* pixelConnect) { //Simple connect pixels
  //This works for our purposes but if I wanted to make this better I could work with slopes.
  int xStep = x2-x1>=0 ? 1 : -1;
  while (x1 != x2) {
    x1 += xStep;
    editPixel(pixels, pixelConnect, y1*rowstride + x1*channels, 0, channels);  
  }
  int yStep = y2-y1>=0 ? 1 : -1;
  while (y1 != y2) {
    y1 += yStep;
    editPixel(pixels, pixelConnect, y1*rowstride + x1*channels, 0, channels);  
  }
} //FUNCTION END

GdkPixbuf* applyBitmap(GdkPixbuf *apply) { //Returns a pixbuf of an image masked by a bitmap
  int width = gdk_pixbuf_get_width(apply), height = gdk_pixbuf_get_height(apply);

  uc* mainPixels = gdk_pixbuf_get_pixels(apply);
  if (width != gdk_pixbuf_get_width(quickSelectBitMap) || height != gdk_pixbuf_get_height(quickSelectBitMap)) { //If the width and height of the apply pixbuf don't match the quickSelectBitMap then we need to resize it
    GdkPixbuf *temp = quickSelectBitMap;
    quickSelectBitMap = gdk_pixbuf_scale_simple(quickSelectBitMap, width, height, GDK_INTERP_BILINEAR);
    g_object_unref(temp);
  }
  //quickSelectBitMap and apply should have the same width and height here
  uc* bitPixels = gdk_pixbuf_get_pixels(quickSelectBitMap);
  int rowstride = gdk_pixbuf_get_rowstride(apply), channels = gdk_pixbuf_get_n_channels(apply);

  //Make a new res pixbuf to return
  GdkPixbuf *res = gdk_pixbuf_new(GDK_COLORSPACE_RGB, (channels == 4 ? TRUE : FALSE), 8, width, height);
  uc* resPixels = gdk_pixbuf_get_pixels(res);

  //If quickSelectBitMap has a pixel that's not black, color it in. Otherwise use the pixel from the apply pixbuf.
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      int offset = i*rowstride + j*channels;
      if (bitPixels[offset] != 0) 
        editPixel(resPixels, bitPixels, offset, offset, channels);
      else
        editPixel(resPixels, mainPixels, offset, offset, channels);
    }
  }
  
  return res;
} //FUNCTION END


void cropImage(GtkWidget *widget, gpointer data) { 
    if (originalImagePix == NULL || (currentImageHeight < scrollHeight || currentImageWidth < scrollWidth) || quickSelectionOn) return;
    
    GtkWidget *scroll = GTK_WIDGET(data);
    GtkAdjustment *hAdj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scroll)), *vAdj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scroll));

    //We set originalImagePix to a resized version of itself that we can use to crop (we need a resizied version of the current zoom since that's how we are referencing pixels)
    currentPix = gdk_pixbuf_scale_simple(originalImagePix, currentImageWidth, currentImageHeight, GDK_INTERP_NEAREST);
    g_object_unref(originalImagePix);
    originalImagePix = gdk_pixbuf_copy(currentPix);
    g_object_unref(currentPix); 

    uc* pixels = gdk_pixbuf_get_pixels(originalImagePix);
    int rowstride = gdk_pixbuf_get_rowstride(originalImagePix), channels = gdk_pixbuf_get_n_channels(originalImagePix);

    //CurrentPix is made into a new pixbuf with the height and width of the scroll box, since that is what we reference to crop and what bounds the image
    currentPix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, (channels == 4 ? TRUE : FALSE), 8, scrollWidth, scrollHeight);
    int rowstrideNew = gdk_pixbuf_get_rowstride(currentPix);
    uc* pixelsNew = gdk_pixbuf_get_pixels(currentPix); 

    int startingPixelWidth = (int)gtk_adjustment_get_value(hAdj), startingPixelHeight = (int)gtk_adjustment_get_value(vAdj);
    for (int i = startingPixelHeight; i < scrollHeight+startingPixelHeight; i++) {
      for (int j = startingPixelWidth; j < scrollWidth+startingPixelWidth; j++) {
        int offsetNew = ((i-startingPixelHeight)*rowstrideNew) + (j-startingPixelWidth)*channels, offset = i*rowstride + j*channels;
        editPixel(pixelsNew, pixels, offsetNew, offset, channels);
      } 
    }
    currentImageHeight = scrollHeight;
    currentImageWidth = scrollWidth;

    g_object_unref(originalImagePix);
    originalImagePix = gdk_pixbuf_copy(currentPix);
    g_object_unref(currentPix); 
    resizeImage(widget, NULL);
    trackModification();
} //FUNCTION END


void modifyImageDialog(GtkWidget *widget, gpointer data) { //Shows the dialog that contains all image modifying functions 
  if (originalImagePix == NULL) return;

  if (quickSelectionOn) {  //This will setup the quickSelectBitMap for coloring
    uc* pixels = gdk_pixbuf_get_pixels(quickSelectBitMap);
    int width = gdk_pixbuf_get_width(quickSelectBitMap), height = gdk_pixbuf_get_height(quickSelectBitMap);
    int rowstride = gdk_pixbuf_get_rowstride(quickSelectBitMap), channels = gdk_pixbuf_get_n_channels(quickSelectBitMap);

    uc **colorMap = malloc(height*sizeof(uc*));
    for (int i = 0; i < height; i++) {
        colorMap[i] = malloc(width*sizeof(uc));
        for (int j = 0; j < width; j++) colorMap[i][j] = 0;
    }

    /*  -----------------------------------LOGIC---------------------------------------------
     *  We will always have a bounded area, so the idea was whenever we encounter a 
     *  white pixel that we will start drawing and when we encounter another white pixel it we will stop
     * 
     *  Bugs occured using this approach, so this is the consensus. We will use the same exact logic
     *  and read in all directions (up,right,down,left), if one of these directions agrees that we must color a slot
     *  it will add one to the colorMap. If 3 or more directions agree we should color then we should in fact color.
     * 
     *  So once we finished the colorMap we read all elements and search for ones that have a value of 3 or more. 
     *  These elements will be placed in their respective spots in the quickSelectBitMap.
     *  Then the quickSelectBitMap will be resized so it can interact with the originalImagePix.
     *  We will quit quick selection once the user closes the dialog to avoid bugs.
     *  
     *  Bascially it's a bunch of linear searches that reach a consensus.
     *  This is what I came up with after going insane for a bit, legit running on pure hope lmao.
     */

    uc* colorPixels = malloc(4*sizeof(uc));
    for (int i = 0; i < 4; i++) colorPixels[i] = quickSelectionColor;
    bool setColorNow = false, colorNow = false;

    //Right Search
    for (int i = 0; i < height; i++) {
      setColorNow = false, colorNow = false;
      for (int j = 0; j < width; j++) {
        int offset = i*rowstride + j*channels;
        if (pixels[offset] != 0) setColorNow = true;
        if (pixels[offset] == 0 && setColorNow) {
          colorNow = !colorNow;
          setColorNow = false;
        }
        if (colorNow) colorMap[i][j] += 1;
      }
    }

    //Left Search
    for (int i = 0; i < height; i++) {
      setColorNow = false, colorNow = false;
      for (int j = width-1; j >= 0; j--) {
        int offset = i*rowstride + j*channels;
        if (pixels[offset] != 0) setColorNow = true;
        if (pixels[offset] == 0 && setColorNow) {
          colorNow = !colorNow;
          setColorNow = false;
        }
        if (colorNow) colorMap[i][j] += 1;
      }
    }
  
    //Down search
    for (int i = 0; i < width; i++) {
      setColorNow = false, colorNow = false;
      for (int j = 0; j < height; j++) {
        int offset = j*rowstride + i*channels;
        if (pixels[offset] != 0) setColorNow = true;
        if (pixels[offset] == 0 && setColorNow) {
          colorNow = !colorNow;
          setColorNow = false;
        }
        if (colorNow) colorMap[j][i] += 1;
      }
    }

    //Up search
    for (int i = 0; i < width; i++) {
      setColorNow = false, colorNow = false;
      for (int j = height-1; j >= 0; j--) {
        int offset = j*rowstride + i*channels;
        if (pixels[offset] != 0) setColorNow = true;
        if (pixels[offset] == 0 && setColorNow) {
          colorNow = !colorNow;
          setColorNow = false;
        }
        if (colorNow) colorMap[j][i] += 1;
      }
    }

    //Edit pixels of quickSelectionBitMap
    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        int offset = i*rowstride + j*channels;
        if (colorMap[i][j] > 2) editPixel(pixels, colorPixels, offset, 0, channels);
      }
    }

    for (int i = 0; i < height; i++) free(colorMap[i]);
    free(colorMap);
    //Please don't ask lmao ;-; (applyBitmap also resizes the bitmap as needed so basically I'm just being lazy here)
    g_object_unref(applyBitmap(originalImagePix));
  }

  GtkWidget *dialog = gtk_dialog_new();
  gtk_widget_set_size_request(dialog, 500, 500);
  gtk_window_set_title(GTK_WINDOW(dialog), "Modifier Functions");

  GtkWidget *grid = gtk_grid_new();

  entry = gtk_entry_new(); //Create entry box
  gtk_widget_set_hexpand(entry, true);
  gtk_grid_attach(GTK_GRID(grid), entry, 0, 0, 1, 1);
  GtkWidget *notebook = gtk_notebook_new(); //Create Notebook
  gtk_grid_attach(GTK_GRID(grid), notebook, 0, 1, 1, 1);

  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);

  //Modifier Button order (0,0) -> (0,1) -> (1,0) -> (1,1)
  //EDITS
  grid = gtk_grid_new();
  modifyButtonCreate(grid, (int[]){0, 0, 1, 1}, &editAllPixels, &((EditArgs){&greyscalePixel, -1, 1}), "Greyscale");
  modifyButtonCreate(grid, (int[]){0, 1, 1, 1}, &editAllPixels, &((EditArgs){&invertPixel, -1, 1}), "Invert");
  modifyButtonCreate(grid, (int[]){1, 0, 1, 1}, &editAllPixels, &((EditArgs){&darkenPixel, 2, 2}), "Darken*");
  modifyButtonCreate(grid, (int[]){1, 1, 1, 1}, &editAllPixels, &((EditArgs){&lightenPixel, 2, 2}), "Lighten*");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, gtk_label_new("Edits"));
  //END OF EDITS

  //EDITS 2
  grid = gtk_grid_new();
  modifyButtonCreate(grid, (int[]){0,0,1,1}, &editAllPixels, &((EditArgs){&swirlImage, 1, 4}), "Swirl*");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, gtk_label_new("Edits 2"));
  //END OF EDITS 2

  //SHIFTS
  grid = gtk_grid_new();
  modifyButtonCreate(grid, (int[]){0, 0, 1, 1}, &flip, NULL, "Flip");
  modifyButtonCreate(grid, (int[]){0, 1, 1, 1}, &rotateRight, NULL, "Rotate Right");
  modifyButtonCreate(grid, (int[]){1, 0, 1, 1}, &flop, NULL, "Flop");
  modifyButtonCreate(grid, (int[]){1, 1, 1, 1}, &rotateLeft, NULL, "Rotate Left");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, gtk_label_new("Shifts"));
  //END OF SHIFTS

  //UTILS
  grid = gtk_grid_new();
  modifyButtonCreate(grid, (int[]){0, 0, 1, 1}, &revertImage, NULL, "Revert");
  modifyButtonCreate(grid, (int[]){0, 1, 1, 1}, &resizeSpecific, NULL, "ResizeX");
  modifyButtonCreate(grid, (int[]){1, 0, 1, 1}, &changeZoomLevel, NULL, "ZoomLevel*");
  modifyButtonCreate(grid, (int[]){1, 1, 1, 1}, &redoImage, NULL, "Redo");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, gtk_label_new("UTILS"));
  //END OF UTILS

  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);

  if (quickSelectionOn) {
    GdkEventKey event;
    event.keyval = GDK_KEY_Escape;
    keyPress(widget, &event, NULL);
  }
} //FUNCTION END

/*
 * gird - grid the widget is part of
 * gridPos - grid parameters as an array
 * *func - function pointer for which function to call when the button is clicked
 * data - extra data to pass into the function when the button is clicked
 * name - name of the button
 */
void modifyButtonCreate(GtkWidget *grid, int gridPos[4], void (*func)(GtkWidget*, gpointer), gpointer data, char* name) { //Creates buttons for the modifyImageDialog
  GtkWidget *btn = gtk_button_new_with_label(name);
  gtk_widget_set_hexpand(btn, true), gtk_widget_set_vexpand(btn, true);
  gtk_grid_attach(GTK_GRID(grid), btn, gridPos[0], gridPos[1], gridPos[2], gridPos[3]);
  g_signal_connect(btn, "clicked", G_CALLBACK(func), data);
} //FUNCTION END


/* ------------------------------------------------------------------------------------------
 * IMAGE MODIFIER FUNCTIONS
 * All image modifier functions exist below here
 * ------------------------------------------------------------------------------------------
*/
void editAllPixels(GtkWidget *widget, gpointer data) { //Used to apply an algorithm to all pixels in the originalImagePix
    EditArgs *args = (EditArgs *)data;

    int factor = atoi(gtk_entry_get_text(GTK_ENTRY(entry)));
    if (factor <= 1) factor = args->factor; //Factor for the operation
    int params = args->params; //Amount of parameters in the operation function

    currentPix = gdk_pixbuf_copy(originalImagePix);
    uc *pixels = gdk_pixbuf_get_pixels(currentPix); //Take pixels from the currentPix

    uc* quickSelectPixels;
    if (quickSelectionOn) quickSelectPixels = gdk_pixbuf_get_pixels(quickSelectBitMap);
    /*
      Get the size of each row and the amount of channels in the image
      If an image only give you rgb that means it has 3 channels such as in a jpg, if it gives you rgba like in a png that means it has 4 channels.
    */
    int rowStride = gdk_pixbuf_get_rowstride(currentPix), channels = gdk_pixbuf_get_n_channels(currentPix);

    int width = gdk_pixbuf_get_width(currentPix), height = gdk_pixbuf_get_height(currentPix); //width and height of the original image

    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        int offset = i * rowStride + j*channels;
        uc* rgb = malloc(3*sizeof(uc));
        editPixel(rgb, pixels, 0, offset, 3); //Edit pixel

        uc* newPixel;
        switch (params) {
           case 1: newPixel=args->operation(rgb);
           break;
           case 2: newPixel=args->operation(rgb, factor);
           break;
           case 4: newPixel=args->operation(rgb, factor, j, i);
        }
        if (quickSelectionOn) {
            if (quickSelectPixels[offset] != 0) editPixel(pixels, newPixel, offset, 0, 3);
        } else { 
          editPixel(pixels, newPixel, offset, 0, 3); //Edit pixel
        }
        free(rgb);
        free(newPixel);
      }
    }
    g_object_unref(originalImagePix);
    originalImagePix = gdk_pixbuf_copy(currentPix);
    g_object_unref(currentPix);
    reDrawImage();
    trackModification();
} //FUNCTION END


uc* greyscalePixel(uc* rgb) { //Greyscale a pixel
  uc* pixel = malloc(3 * sizeof(uc));
  pixel[0] = pixel[1] = pixel[2] = (rgb[0]+rgb[1]+rgb[2])/3;
  return pixel;
} //FUNCTION END

uc* invertPixel(uc* rgb) { //Invert a pixel
  uc* pixel = malloc(3 * sizeof(uc));
  pixel[0] = 255 - rgb[0];
  pixel[1] = 255 - rgb[1];
  pixel[2] = 255 - rgb[2];
  return pixel;
} //FUNCTION END

uc* darkenPixel(uc* rgb, int factor) { //Darken a pixel
  uc* pixel = malloc(3 * sizeof(uc));
  pixel[0] = (int)(pow(rgb[0]/255.0, (double)factor)*255);
  pixel[1] = (int)(pow(rgb[1]/255.0, (double)factor)*255);
  pixel[2] = (int)(pow(rgb[2]/255.0, (double)factor)*255);
  return pixel;
} //FUNCTION END

uc* lightenPixel(uc* rgb, int factor) { //Lighten a pixel
  uc* pixel = malloc(3 * sizeof(uc));
  pixel[0] = (int)(pow(rgb[0]/255.0, 1/(double)factor)*255);
  pixel[1] = (int)(pow(rgb[1]/255.0, 1/(double)factor)*255);
  pixel[2] = (int)(pow(rgb[2]/255.0, 1/(double)factor)*255);
  return pixel;
} //FUNCTION END
uc* swirlImage(uc* rgb, int factor, int x, int y) { //Swirl the image
  int width = gdk_pixbuf_get_width(originalImagePix), height = gdk_pixbuf_get_height(originalImagePix);
  double dx = x - width/2.0; //Distance away from middle on the x
  double dy = y - height/2.0; //Distance away from middle on the y
  double angle = atan2(dy, dx);
  double radius = sqrt(dx*dx + dy*dy);
  double swirl = radius * (factor/1000.0);
  
  int sourceX = (int)(radius * cos(angle + swirl) + width / 2.0);
  int sourceY = (int)(radius * sin(angle + swirl) + height / 2.0);

  uc* pixel = malloc(3 * sizeof(uc));
  if (sourceX >= 0 && sourceX < width && sourceY >= 0 && sourceY < height) {
    uc* originalPixels = gdk_pixbuf_get_pixels(originalImagePix) + sourceY*gdk_pixbuf_get_rowstride(originalImagePix) + sourceX*gdk_pixbuf_get_n_channels(originalImagePix); //Pointer Arithmatic
    editPixel(pixel, originalPixels, 0, 0, 3);
  } else {
    editPixel(pixel, rgb, 0, 0, 3);
  }
  return pixel;
} //FUNCTION END


void flip(GtkWidget *widget, gpointer data) { //Flip the image
  if (quickSelectionOn) return;

  uc *pixels = gdk_pixbuf_get_pixels(originalImagePix);
  int rowstride = gdk_pixbuf_get_rowstride(originalImagePix), channels = gdk_pixbuf_get_n_channels(originalImagePix);
  int width = gdk_pixbuf_get_width(originalImagePix), height = gdk_pixbuf_get_height(originalImagePix);

  currentPix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, (channels == 4 ? TRUE : FALSE), 8, width, height);
  uc *pixelsNew = gdk_pixbuf_get_pixels(currentPix);

  /* (How the loop works)
    Imagine an image like this

    ----
    ----
    &*** (We loop through this row first from 0 to n rightward and then go up a row)

    - represent image pixels
    * represent pixels we are looping through
    & represents the pixel we start at when looping
  */
  for (int i = height-1; i >= 0; i--) {
    for (int j = 0; j < width; j++) {
      int offsetNew = ((height-1-i)*rowstride) + j*channels, offset = i*rowstride + j*channels;
      editPixel(pixelsNew, pixels, offsetNew, offset, channels); //Edit pixel
    }
  }
  g_object_unref(originalImagePix);
  originalImagePix = gdk_pixbuf_copy(currentPix);
  g_object_unref(currentPix);
  resizeImage(widget, NULL);
  trackModification();
} //FUNCTION END


void flop(GtkWidget *widget, gpointer data) { //Flop the image
  if (quickSelectionOn) return;

  uc* pixels = gdk_pixbuf_get_pixels(originalImagePix);
  int rowstride = gdk_pixbuf_get_rowstride(originalImagePix), channels = gdk_pixbuf_get_n_channels(originalImagePix);
  int width = gdk_pixbuf_get_width(originalImagePix), height = gdk_pixbuf_get_height(originalImagePix);

  currentPix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, (channels == 4 ? TRUE : FALSE), 8, width, height);
  uc* pixelsNew = gdk_pixbuf_get_pixels(currentPix);
  /* (How the loop works)
    Imagine an image like this

    ---& (We loop through this column first from 0 to n downward and then go left a column)
    ---*
    ---* 

    - represent image pixels
    * represent pixels we are looping through
    & represents the pixel we start at when looping
  */
  for (int i = width - 1; i >= 0; i--) {
    for (int j = 0; j < height; j++) {
      int offsetNew = j*rowstride + (width-1-i)*channels, offset = j*rowstride + i*channels;
      editPixel(pixelsNew, pixels, offsetNew, offset, channels); //Edit Pixel
    }
  }
  g_object_unref(originalImagePix);
  originalImagePix = gdk_pixbuf_copy(currentPix);
  g_object_unref(currentPix);
  resizeImage(widget, NULL);
  trackModification();
} //FUNCTION END


void rotateRight(GtkWidget *widget, gpointer data) { //Rotate an image right
  if (quickSelectionOn) return;

  uc* pixels = gdk_pixbuf_get_pixels(originalImagePix);
  int rowstride = gdk_pixbuf_get_rowstride(originalImagePix), channels = gdk_pixbuf_get_n_channels(originalImagePix);
  int width = gdk_pixbuf_get_width(originalImagePix), height = gdk_pixbuf_get_height(originalImagePix);

  currentPix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, (channels == 4 ? TRUE : FALSE), 8, height, width);
  int rowstrideNew = gdk_pixbuf_get_rowstride(currentPix);
  uc* pixelsNew = gdk_pixbuf_get_pixels(currentPix); 
  
  /* (How the loop works)
    Imagine an image like this

    &****                                            ----&
    -----                                            ----*
    ----- (Then we transfer the pixels like this) -> ----*
    -----                                            ----*
    -----                                            ----*

    - represent image pixels
    * represent pixels we are looping through
    & represents the pixel we start at when looping
  */
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      //Keep in mind for the currentPix the width and height have swapped
      int offsetNew = j*rowstrideNew + (height-1-i)*channels, offset = i*rowstride + j*channels;
      editPixel(pixelsNew, pixels, offsetNew, offset, channels); //Edit Pixel
    }
  }
  g_object_unref(originalImagePix);
  originalImagePix = gdk_pixbuf_copy(currentPix);
  g_object_unref(currentPix);
  resizeImage(widget, NULL);
  trackModification();
} //FUNCTION END


void rotateLeft(GtkWidget *widget, gpointer data) { //Rotates an image left
  if (quickSelectionOn) return;

  uc* pixels = gdk_pixbuf_get_pixels(originalImagePix);
  int rowstride = gdk_pixbuf_get_rowstride(originalImagePix), channels = gdk_pixbuf_get_n_channels(originalImagePix);
  int width = gdk_pixbuf_get_width(originalImagePix), height = gdk_pixbuf_get_height(originalImagePix);

  currentPix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, (channels == 4 ? TRUE : FALSE), 8, height, width);
  int rowstrideNew = gdk_pixbuf_get_rowstride(currentPix);
  uc* pixelsNew = gdk_pixbuf_get_pixels(currentPix);
  /* (How the loop works)
    Imagine an image like this

    &****                                            *----
    -----                                            *----
    ----- (Then we transfer the pixels like this) -> *----
    -----                                            *----
    -----                                            &----

    - represent image pixels
    * represent pixels we are looping through
    & represents the pixel we start at when looping
  */
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      //Keep in mind for the currentPix the width and height have swapped
      int offsetNew = (width-1-j)*rowstrideNew+i*channels, offset = i*rowstride + j*channels;
      editPixel(pixelsNew, pixels, offsetNew, offset, channels); //Edit Pixel
    }
  }
  g_object_unref(originalImagePix);
  originalImagePix = gdk_pixbuf_copy(currentPix);
  g_object_unref(currentPix);
  resizeImage(widget, NULL);
  trackModification();
} //FUNCTION END


void trackModification() { //Tracks the amount of modifications done to the image
  char filePath[30]; //SaveContent/image.jpeg is 22 characters + the null terminator making it 23, so you can have up to 10^7 - 1 (7 digits max) image saves lol

  for (int i = modificationCounter+1; i <= maxRedo; i++) { //Since we just made a new modification we should no longer be able to redo (Get rid of all images saved for redo)
    sprintf(filePath, SAVE_CONTENT_PATH, i);
    remove(filePath);
  }
  maxRedo = 0;
  strcpy(filePath, ""); //Clear filepath array

  sprintf(filePath, SAVE_CONTENT_PATH, ++modificationCounter);
  gdk_pixbuf_save(originalImagePix, filePath, "jpeg", NULL, NULL);
} //FUNCTION END

void revertImage(GtkWidget *widget, gpointer data) { //revert image to older version (undo function)
  if (quickSelectionOn) return;

  if (maxRedo == 0) maxRedo = modificationCounter; //Save the max redo

  g_object_unref(originalImagePix);
  if (modificationCounter >= 1) modificationCounter--; //Alter modification counter for undo

  if (modificationCounter < 1) {
    originalImagePix = gdk_pixbuf_new_from_file(orignalImagePath, NULL); //Sets the originalImagePix to the actual originally selected image
  } else {
    char filePath[30]; //SaveContent/image.jpeg is 22 characters + the null terminator making it 23, so you can have up to 10^7 - 1 (7 digits max) image saves lol
    sprintf(filePath, SAVE_CONTENT_PATH, modificationCounter);
  
    originalImagePix = gdk_pixbuf_new_from_file(filePath, NULL);
  }
  resizeImage(widget, NULL);
} //FUNCTION END
void redoImage(GtkWidget *widget, gpointer data) { //Brings back reverted image (redo function)
  if (quickSelectionOn) return;

  if (modificationCounter < maxRedo) {
    g_object_unref(originalImagePix);

    char filePath[30];
    sprintf(filePath, SAVE_CONTENT_PATH, ++modificationCounter);
    originalImagePix = gdk_pixbuf_new_from_file(filePath, NULL);
    resizeImage(widget, NULL);
  }
} //FUNCTION END

void resizeSpecific(GtkWidget *widget, gpointer data) { //Resizies an image to specific dimensions (Changes original image for saving purposes)
  if (quickSelectionOn) return;

  char *text = (char*)gtk_entry_get_text(GTK_ENTRY(entry));
  if (strchr(text, 'x') == NULL) return;

  int width = atoi(text); //Gives the number before x in e.g 500x500
  int height = atoi(strchr(text, 'x')+1); //Gives the number after x in e.g 500x500
  if (width <= 0 || height <= 0) return;

  currentPix = gdk_pixbuf_scale_simple(originalImagePix, width, height, GDK_INTERP_NEAREST);

  g_object_unref(originalImagePix);
  originalImagePix = gdk_pixbuf_copy(currentPix);
  g_object_unref(currentPix);

  gtk_image_set_from_pixbuf(GTK_IMAGE(image), originalImagePix);

  currentImageWidth = width;
  currentImageHeight = height;
} //FUNCTION END


void changeZoomLevel(GtkWidget *widget, gpointer data) { //Changes the zoom level for when you scroll on an image
    int newLevel = atoi(gtk_entry_get_text(GTK_ENTRY(entry)));
    if (newLevel >= 1 && newLevel <= 1000) zoomLevel = newLevel; 
} //FUNCTION END


void editPixel(uc *pixelsNew, uc* pixelsOld, int offsetNew, int offsetOld, int channels) { //Used to set a pixel
  for (int i = 0; i < channels; i++) pixelsNew[offsetNew+i] = pixelsOld[offsetOld+i];
  /* The for loop above is equivalent to doing this
  
    pixelsNew[offsetNew] = pixelsOld[offsetOld];
    pixelsNew[offsetNew + 1] = pixelsOld[offsetOld + 1];
    pixelsNew[offsetNew + 2] = pixelsOld[offsetOld + 2];
    if (channels == 4) pixelsNew[offsetNew + 3] = pixelsOld[offsetOld + 3];
  */
} //FUNCTION END

