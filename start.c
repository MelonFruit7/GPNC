#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

typedef unsigned char uc;
typedef struct { //This struct is used so we can successfully pass in parameters into the editAllPixels function via a g_signal_connect
  uc* (*operation)(uc*, int);
  int factor;
} EditArgs;

//Image with original aspect ratio
GdkPixbuf *originalImagePix = NULL;
//Image placeholder
GdkPixbuf *currentPix = NULL;
//Current image width and height (resized)
int currentImageWidth, currentImageHeight;

//The image container
GtkWidget *image;
//Check Button which maintains aspect ratio if selected
GtkWidget *keepAspectRatio;
//The scrolled window widget width and height 
int scrollWidth = 725, scrollHeight = 725;

//Color Button
GtkWidget *colorBtn;
//Entry box where users can type scale factors when modifying an image
GtkWidget *entry;

//Are we currently moving through our image?
bool movingImage = false;
//These are initially set in "onImageClick()", they represent the previous position of the mouse while dragging through the image
int xMove, yMove;


void windowResized(GtkWidget *widget, GdkRectangle *allocation, gpointer data); //Call when the window is resized
void resizeImage(GtkWidget *widget, gpointer data); //Call when the window is resized
void chooseFile(GtkWidget *widget, gpointer data); //Function to choose a file
void defineCSS(GtkWidget *widget, GtkCssProvider *cssProvider, char* class); //Function used to define css for widgets
bool scrolledResize(GtkWidget *widget, GdkEventScroll *event, gpointer data); //Used to resize the image on scroll
bool onImageClick(GtkWidget *widget, GdkEventButton *event, gpointer data); //Called when you click the image
bool onImageRelease(GtkWidget *widget, GdkEventButton *event, gpointer data); //Called when you release the image
bool onImageMove(GtkWidget *widget, GdkEventMotion *event, gpointer data); //Called while you are dragging the image
void modifyImageDialog(GtkWidget *widget, gpointer data); //Called when user wants to modify the image
void modifyButtonCreate(GtkWidget *grid, int gridPos[4], void (*func)(GtkWidget*, gpointer), gpointer data, char* name); //Creates buttons for the modifyImageDialog

//Modify image functions
void editAllPixels(GtkWidget *widget, gpointer data);
uc* greyscalePixel(uc* rgb, int factor);
uc* invertPixel(uc* rgb, int factor);
uc* darkenPixel(uc* rgb, int factor);
uc* lightenPixel(uc* rgb, int factor);
void flip(GtkWidget *widget, gpointer data);
void flop(GtkWidget *widget, gpointer data);
void rotateRight(GtkWidget *widget, gpointer data);
void rotateLeft(GtkWidget *widget, gpointer data);
void editPixel(uc *pixelsNew, uc* pixelsOld, int offsetNew, int offsetOld, int channels);

void activate(GtkApplication *app, gpointer data) {
  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_default_size(GTK_WINDOW(window), scrollWidth, scrollHeight);
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);
  gtk_window_set_title(GTK_WINDOW(window), "Image Modifier");

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
  //Aspect Ratio Check
  keepAspectRatio = gtk_check_button_new_with_label("Keep Aspect Ratio");
  gtk_box_pack_start(GTK_BOX(hbox), keepAspectRatio, FALSE, FALSE, 0);

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
  g_signal_connect(fileChooser, "clicked", G_CALLBACK(chooseFile), image);
  g_signal_connect(resizeBtn, "clicked", G_CALLBACK(resizeImage), NULL);
  g_signal_connect(window, "size-allocate", G_CALLBACK(windowResized), scrollBox);
  g_signal_connect(scrollBox, "scroll-event", G_CALLBACK(scrolledResize), NULL);
  g_signal_connect(eventBox, "button-press-event", G_CALLBACK(onImageClick), NULL);
  g_signal_connect(eventBox, "button-release-event", G_CALLBACK(onImageRelease), NULL);
  g_signal_connect(scrollBox, "motion-notify-event", G_CALLBACK(onImageMove), NULL);
  g_signal_connect(modifyBtn, "clicked", G_CALLBACK(modifyImageDialog), NULL);
  


  
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
  return ret;
}


void defineCSS(GtkWidget *widget, GtkCssProvider *cssProvider, char* class) { //Used to define css for a widget
  GtkStyleContext *context = gtk_widget_get_style_context(widget);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  gtk_style_context_add_class(context, class);
} //FUNCTION END

void chooseFile(GtkWidget *widget, gpointer data) { //Called when the choose file button is clicked
  GtkWidget *image = data;

  GtkFileChooser *chooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(gtk_widget_get_toplevel(image)),GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL,"_Open", GTK_RESPONSE_ACCEPT, NULL));
  int res = gtk_dialog_run(GTK_DIALOG(chooser));
  if (res == GTK_RESPONSE_ACCEPT) {
    
    //If the originalImagePix exists unref it since we reset it here
    if (originalImagePix != NULL) g_object_unref(originalImagePix);

    //Grab the file path that was given by the file chooser dialog
    char *filePath = gtk_file_chooser_get_filename(chooser);
    //Use the file path to load and image
    originalImagePix = gdk_pixbuf_new_from_file(filePath, NULL);
    //If it wasn't an image then break the program lol
    if (originalImagePix == NULL) exit(1);

    bool keepRatio = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(keepAspectRatio));
    //Get the current width and height of the image and get the aspect ratio
    int width = gdk_pixbuf_get_width(originalImagePix), height = gdk_pixbuf_get_height(originalImagePix);
    double aspectR = (double)width/height;
    double sAspectR = (double)scrollWidth/scrollHeight;


    //Here we make sure if we are keeping the aspect ratio we also make the image larger than the scollBox
    width = scrollWidth, height = (int)(width/aspectR);
    //If the aspect ratio (w/h) of the image is greater than the aspect ratio (w/h) of the screen, it means we have to set the image in relation to the height rather than the width to make it take up the entire screen.
    if (aspectR > sAspectR) height = scrollHeight, width = (int)(height*aspectR);

    //If we don't keep the aspect ratio just make it the same size as the scroll window
    if (!keepRatio) {width = scrollWidth, height = scrollHeight;}

    //Resize the pixbuf image
    currentPix = gdk_pixbuf_scale_simple(originalImagePix, width, height, GDK_INTERP_NEAREST);

    //Set the pixbuf to the image view
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), currentPix);

    currentImageWidth = width;
    currentImageHeight = height;

    g_object_unref(currentPix);
    g_free(filePath);
  }

  gtk_widget_destroy(GTK_WIDGET(chooser));
} //FUNCTION END

void resizeImage(GtkWidget *widget, gpointer data) { //Called when the resize button is clicked
  if (originalImagePix == NULL) return;
  
  bool keepRatio = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(keepAspectRatio));
  //Get the current width and height of the image and get the aspect ratio
  int width = gdk_pixbuf_get_width(originalImagePix), height = gdk_pixbuf_get_height(originalImagePix);
  double aspectR = (double)width/height;
  double sAspectR = (double)scrollWidth/scrollHeight;

  width = scrollWidth, height = (int)(width/aspectR);
  if (aspectR > sAspectR) height = scrollHeight, width = (int)(height*aspectR);

  if (!keepRatio) {width = scrollWidth, height = scrollHeight;}

  currentPix = gdk_pixbuf_scale_simple(originalImagePix, width, height, GDK_INTERP_NEAREST);
  gtk_image_set_from_pixbuf(GTK_IMAGE(image), currentPix);

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

  //Sometimes delta_y starts at 0 for no reason and it causes weird behavior in the program ;-;
  if (originalImagePix == NULL || event->delta_y == 0) return 1;

  bool zoomIn = event->delta_y <= 0;
  if ((currentImageHeight > 3000 || currentImageWidth > 3000) && zoomIn) {return 1;} else if((currentImageHeight <= 40 || currentImageWidth <= 40) && !zoomIn) {return 1;}

  double aspectR = (double)currentImageWidth/currentImageHeight;
  int add = zoomIn ? 40 : -40;

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

    This works fine, but it isn't perfect.
    ------------------------------------------------------------------------------------
  */
  double displacementV = add*(event->y/scrollHeight), displacementH = ((int)(add*aspectR))*(event->x/scrollWidth);


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
  if (event->button == GDK_BUTTON_PRIMARY) {
    movingImage = true;
    xMove = event->x;
    yMove = event->y;

    if (originalImagePix != NULL) {
      currentPix = gdk_pixbuf_scale_simple(originalImagePix, currentImageWidth, currentImageHeight, GDK_INTERP_NEAREST);
      uc *pixels = gdk_pixbuf_get_pixels(currentPix);
      int rowstride = gdk_pixbuf_get_rowstride(currentPix), channels = gdk_pixbuf_get_n_channels(currentPix);
      GdkRGBA color;
      int offset = yMove*rowstride + xMove*channels;
      color.red = pixels[offset]/255.0;
      color.green = pixels[offset+1]/255.0;
      color.blue = pixels[offset+2]/255.0;
      color.alpha = 1.0;
      gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(colorBtn), &color);
      g_object_unref(currentPix);
    }

  }
  return 1;
} //FUNCTION END

bool onImageRelease(GtkWidget *widget, GdkEventButton *event, gpointer data) { //Called after you release a click on the image
  if (event->button == GDK_BUTTON_PRIMARY) movingImage = false;
  return 1;
} //FUNCTION END

bool onImageMove(GtkWidget *widget, GdkEventMotion *event, gpointer data) { //Captures Mouse movement through the image
  if (movingImage) {
    GtkAdjustment *hAdj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(widget));
    GtkAdjustment *vAdj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(widget));

    int adjX = xMove - event->x;
    int adjY = yMove - event->y;

    //Moves scroll wheel based on where our original mouse position was
    gtk_adjustment_set_value(hAdj, gtk_adjustment_get_value(hAdj)+adjX); 
    gtk_adjustment_set_value(vAdj, gtk_adjustment_get_value(vAdj)+adjY);
  }
  return 1;
} //FUNCTION END


void modifyImageDialog(GtkWidget *widget, gpointer data) { //Shows the dialog that contains all image modifying functions 
  if (originalImagePix == NULL) return;

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
  //PAGE 1 
  grid = gtk_grid_new();
  modifyButtonCreate(grid, (int[]){0, 0, 1, 1}, &editAllPixels, &((EditArgs){&greyscalePixel, -1}), "Greyscale");
  modifyButtonCreate(grid, (int[]){0, 1, 1, 1}, &editAllPixels, &((EditArgs){&invertPixel, -1}), "Invert");
  modifyButtonCreate(grid, (int[]){1, 0, 1, 1}, &editAllPixels, &((EditArgs){&darkenPixel, 2}), "Darken*");
  modifyButtonCreate(grid, (int[]){1, 1, 1, 1}, &editAllPixels, &((EditArgs){&lightenPixel, 2}), "Lighten*");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, gtk_label_new("Page 1"));
  //END OF PAGE 1

  //PAGE 2
  grid = gtk_grid_new();
  modifyButtonCreate(grid, (int[]){0, 0, 1, 1}, &flip, NULL, "Flip");
  modifyButtonCreate(grid, (int[]){0, 1, 1, 1}, &rotateRight, NULL, "Rotate Right");
  modifyButtonCreate(grid, (int[]){1, 0, 1, 1}, &flop, NULL, "Flop");
  modifyButtonCreate(grid, (int[]){1, 1, 1, 1}, &rotateLeft, NULL, "Rotate Left");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, gtk_label_new("Page 2"));
  //END OF PAGE 2


  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);
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


/* ---------------------------------------------
 * IMAGE MODIFIER FUNCTIONS
 * All image modifier functions exist below here
 * --------------------------------------------- 
*/
void editAllPixels(GtkWidget *widget, gpointer data) { //Used to apply an algorithm to all pixels in the originalImagePix
    EditArgs *args = (EditArgs *)data;

    int factor = atoi(gtk_entry_get_text(GTK_ENTRY(entry)));
    if (factor <= 1) factor = args->factor;

    uc *pixels = gdk_pixbuf_get_pixels(originalImagePix); //Take pixels from the originalImagePix
    /*
      Get the size of each row and the amount of channels in the image
      If an image only give you rgb that means it has 3 channels such as in a jpg, if it gives you rgba like in a png that means it has 4 channels.
    */
    int rowStride = gdk_pixbuf_get_rowstride(originalImagePix), channels = gdk_pixbuf_get_n_channels(originalImagePix);

    int width = gdk_pixbuf_get_width(originalImagePix), height = gdk_pixbuf_get_height(originalImagePix); //width and height of the original image

    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        int offset = i * rowStride + j*channels;
        uc* rgb = malloc(3*sizeof(uc));
        editPixel(rgb, pixels, 0, offset, 3); //Edit pixel

        uc* newPixel = args->operation(rgb, factor);
        editPixel(pixels, newPixel, offset, 0, 3); //Edit pixel
        free(rgb);
        free(newPixel);
      }
    }
    resizeImage(widget, NULL);
} //FUNCTION END


uc* greyscalePixel(uc* rgb, int factor) { //Greyscale a pixel
  uc* pixel = malloc(3 * sizeof(uc));
  pixel[0] = pixel[1] = pixel[2] = (rgb[0]+rgb[1]+rgb[2])/3;
  return pixel;
} //FUNCTION END

uc* invertPixel(uc* rgb, int factor) { //Invert a pixel
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


void flip(GtkWidget *widget, gpointer data) { //Flip the image
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
} //FUNCTION END


void flop(GtkWidget *widget, gpointer data) { //Flop the image
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
} //FUNCTION END


void rotateRight(GtkWidget *widget, gpointer data) { //Rotate an image right
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
} //FUNCTION END


void rotateLeft(GtkWidget *widget, gpointer data) { //Rotates an image left
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


