#include "pebble.h"

////// Macros /////////////////
#define MIN(a,b) (a < b ? a : b)
#define MAX(a,b) (a > b ? a : b)

#define TOP_SAND_GRAIN_SIZE 6
#define BOTTOM_SAND_GRAIN_SIZE 4
#define WATCH_HEIGHT 168
#define WATCH_WIDTH 144

////// Global Variables /////////////////

static Window *window;
static Layer *windowLayer;

static InverterLayer *inverterLayer;

static GBitmap * backgroundImage;
static BitmapLayer *backgroundLayer;

static Layer *displayLayer;

int sandGrainTick = 0;

////// Global Consts ////////////////////

const int numTopSandGrains = 24;
const int numBottomSandGrains = 59;

const int sandAnimationColumnX = 70;
const int sandAnimationColumnY = 71;

const int xyTopSandGrains[] = {
  // top line
  45, 29, // 0
  57, 29, // 1
  69, 29, // 2
  81, 29, // 3
  93, 29, // 4
  // second line
  51, 35, // 5
  63, 35, // 6
  75, 35, // 7
  87, 35, // 8
  // third line
  45, 41, // 9
  57, 41, // 10
  69, 41, // 11
  81, 41, // 12
  93, 41, // 13
  // fourth line
  51, 47, // 14
  63, 47, // 15
  75, 47, // 16
  87, 47, // 17
  // fifth line
  57, 53, // 18
  69, 53, // 19
  81, 53, // 20
  // sixth line
  63, 59, // 21
  75, 59, // 22
  // last line
  69, 65 // 23
};

const int dropTopSandGrainOrder[] = {
  2, 1, 3, 0, 4, // top line
  6, 7, 5, 8, // second line
  11, 10, 12, 9, 13, // third line
  15, 16, 14, 17, // fourth line
  19, 18, 20, // fifth line
  21, 22, // sixth line
  23 // last line
};

const int xyBottomSandGrains[] = {
  // zeroth line
  70, 89, // 0
  // top line
  70, 97, // 1
  // second line
  66, 101, // 2
  74, 101, // 3
  // third line
  62, 105, // 4
  70, 105, // 5
  78, 105, // 6
  // fourth line
  58, 109, // 7
  66, 109, // 8
  74, 109, // 9
  82, 109, // 10
  // fifth line
  54, 113, // 11
  62, 113, // 12
  70, 113, // 13
  78, 113, // 14
  86, 113, // 15
  // sixth line
  50, 117, // 16
  58, 117, // 17
  66, 117, // 18
  74, 117, // 19
  82, 117, // 20
  90, 117, // 21
  // seventh line
  46, 121, // 22
  54, 121, // 23
  62, 121, // 24
  70, 121, // 25
  78, 121, // 26
  86, 121, // 27
  94, 121, // 28
  // eighth line
  42, 125, // 29
  50, 125, // 30
  58, 125, // 31
  66, 125, // 32
  74, 125, // 33
  82, 125, // 34
  90, 125, // 35
  98, 125, // 36
  // ninth line
  46, 129, // 37
  54, 129, // 38
  62, 129, // 39
  70, 129, // 40
  78, 129, // 41
  86, 129, // 42
  94, 129, // 43
  // tenth line
  42, 133, // 44
  50, 133, // 45
  58, 133, // 46
  66, 133, // 47
  74, 133, // 48
  82, 133, // 49
  90, 133, // 50
  98, 133, // 51
  // last line
  46, 137, // 52
  54, 137, // 53
  62, 137, // 54
  70, 137, // 55
  78, 137, // 56
  86, 137, // 57
  94, 137, // 58
  
};

const int fillBottomSandGrainOrder[] = {
  55, 54, 56, 53, 57, 52, 58, // bottom line
  47, 48, 46, 49, 45, 50, 44, 51, // second line
  40, 39, 41, 38, 42, 37, 43, // third line
  32, 33, 31, 34, 30, 35, 29, 36, // fourth line
  25, 24, 26, 23, 27, 22, 28, // fifth line,
  18, 19, 17, 20, 16, 21, // sixth line
  13, 12, 14, 11, 15, // seventh line
  8, 9, 7, 10, // eighth line
  5, 4, 6, // ninth line
  2, 3, // tenth line
  1, // top line
  0 // zeroth line
};

const int bottomSandGrainRowAnimationEnd[] = {
  137, 137, 137, 137, 137, 137, 137, // bottom line
  133, 133, 133, 133, 133, 133, 133, 133, // second line
  129, 129, 129, 129, 129, 129, 129, // third line
  125, 125, 125, 125, 125, 125, 125, 125, // fourth line
  121, 121, 121, 121, 121, 121, 121, // fifth line,
  117, 117, 117, 117, 117, 117, // sixth line
  113, 113, 113, 113, 113, // seventh line
  109, 109, 109, 109, // eighth line
  105, 105, 105, // ninth line
  101, 101, // tenth line
  97, // top line
  89 // zeroth line
};

////// Callback Functions /////////////////

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  
  // Tell the application to rerender the layer.
  layer_mark_dirty(displayLayer);
}

static void displayLayer_update_callback(Layer *me, GContext* ctx) {
  // If we're not using the layer, this will prevent compile errors.
  (void)me;
  
  // Getting the time
  time_t now = time(NULL);
  struct tm *time = localtime(&now);
  const int minutes = time->tm_min;
  const int hours   = time->tm_hour;
  
  // Drawing the top sand grains. We use the 'dropTopSandGrainOrder' array to determine
  // what grains are drawn according the hour. This is set up so the grains will empty
  // from the top down. The 'xyTopSandGrains' array gives the co-ordinates of each
  // grain to be drawn.
  int numTopGrainsToDraw = hours;
  for(int grain = numTopGrainsToDraw; grain < numTopSandGrains; grain++) {
    
    if(grain < 0 || grain >= numTopSandGrains) // For safety.
      continue;
    
    const int grainX = xyTopSandGrains[dropTopSandGrainOrder[grain]*2 + 0];
    const int grainY = xyTopSandGrains[dropTopSandGrainOrder[grain]*2 + 1];
    
    graphics_fill_rect(ctx, GRect(grainX, grainY, TOP_SAND_GRAIN_SIZE, TOP_SAND_GRAIN_SIZE), 0, GCornerNone);
  }
  
  // Drawing the bottom sand grains. Same as the above, but instead of emptying, we're
  // filling them up.
  int numBottomGrainsToDraw = minutes;
  for(int grain=0; grain < numBottomGrainsToDraw; grain++) {

    if(grain < 0 || grain >= numBottomSandGrains) // For safety.
      continue;
    
    const int grainX = xyBottomSandGrains[fillBottomSandGrainOrder[grain]*2 + 0];
    const int grainY = xyBottomSandGrains[fillBottomSandGrainOrder[grain]*2 + 1];
    
    graphics_fill_rect(ctx, GRect(grainX, grainY, BOTTOM_SAND_GRAIN_SIZE, BOTTOM_SAND_GRAIN_SIZE), 0, GCornerNone);
  }
  
  // Draw sand animation. The idea here is that we create a column of partially drawn sand grains, and modulate the
  // rows of pixels to give the effect that they're travelling downwards.
  graphics_context_set_fill_color(ctx, GColorBlack);
  
  // Safely getting the y co-ord of how far down we want to draw the sand animation
  // from the 'bottomSandGrainRowAnimationEnd' array.
  const int animationEndIndex = MAX(0, MIN( (numBottomSandGrains-1), numBottomGrainsToDraw));
  const int animationEndY = bottomSandGrainRowAnimationEnd[animationEndIndex];
  
  // We keep a counter for the next vertical position to draw the next grain of falling sand.
  int sandAnimationYCounter = sandAnimationColumnY;
  // To keep a space between each grain, we swap between even (black) and odd (white) grains.
  bool drawOddGrain = true;
  
  // Variables used in modulating the sand grain rows.
  int animatedGrainYStart;
  int animatedGrainYStop;
  
  // Start drawing the column of sand grains.
  while (true) {

    if(drawOddGrain) {
      animatedGrainYStart = MAX(0, sandGrainTick - BOTTOM_SAND_GRAIN_SIZE);
      animatedGrainYStop  = MIN(BOTTOM_SAND_GRAIN_SIZE, sandGrainTick);
    }
    else {
      animatedGrainYStart = sandGrainTick < BOTTOM_SAND_GRAIN_SIZE ? sandGrainTick : 0;
      animatedGrainYStop  = sandGrainTick < BOTTOM_SAND_GRAIN_SIZE ? BOTTOM_SAND_GRAIN_SIZE : sandGrainTick - BOTTOM_SAND_GRAIN_SIZE;
    }
    
    // Making sure we're not drawing outside our bounds
    animatedGrainYStop = MIN(animationEndY, (sandAnimationYCounter + animatedGrainYStop)) - sandAnimationYCounter;
    
    // Stop drawing sand grains if we're outside the bounds
    if((sandAnimationYCounter + animatedGrainYStart) > animationEndY)
      break;
    
    // Draw a single sand grain
    graphics_fill_rect(ctx, GRect(sandAnimationColumnX, sandAnimationYCounter + animatedGrainYStart, BOTTOM_SAND_GRAIN_SIZE, (animatedGrainYStop - animatedGrainYStart)), 0, GCornerNone);
    
    // Switch to drawing the 'other' grain
    drawOddGrain = !drawOddGrain;
    
    // Increment the sand animation counter
    sandAnimationYCounter += BOTTOM_SAND_GRAIN_SIZE;
  }
  
  // Finally, increment the modulation counter.
  sandGrainTick = (sandGrainTick+1) % (BOTTOM_SAND_GRAIN_SIZE*2);
}

static void handle_init() {

  // Initialise the main window
  window = window_create();
  windowLayer = window_get_root_layer(window);
  window_stack_push(window, true /* Animated */);

  // Set up our background bitmap, pulled from our resources.
  backgroundImage=gbitmap_create_with_resource(RESOURCE_ID_HOUR_GLASS_BG);
  backgroundLayer=bitmap_layer_create(layer_get_frame(windowLayer));
  bitmap_layer_set_bitmap(backgroundLayer, backgroundImage);
  layer_add_child(windowLayer, bitmap_layer_get_layer(backgroundLayer));
  
  // Init the layer for the display.
  displayLayer=layer_create(layer_get_frame(windowLayer));
  layer_set_update_proc(displayLayer, displayLayer_update_callback);
  layer_add_child(windowLayer, displayLayer);
  
  // Lastly, set up the in inversion layer to make everything
  // nice and dark.
  inverterLayer=inverter_layer_create(GRect(0, 0, WATCH_WIDTH, WATCH_HEIGHT)); 
  layer_add_child(windowLayer, inverter_layer_get_layer(inverterLayer));

  // tick tock
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void handle_deinit() {
  tick_timer_service_unsubscribe();

  // Very important, destroy our layers, bitmap, & windows when we're done with them.
  layer_remove_from_parent(inverter_layer_get_layer(inverterLayer));
  inverter_layer_destroy(inverterLayer);

  layer_remove_from_parent(displayLayer);
  layer_destroy(displayLayer);

  layer_remove_from_parent(bitmap_layer_get_layer(backgroundLayer));
  bitmap_layer_destroy(backgroundLayer);
  gbitmap_destroy(backgroundImage);

  window_destroy(window);
}

////// Watchface Entry-point /////////////////

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
