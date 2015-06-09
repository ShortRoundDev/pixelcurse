#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#define CONTRAST 140

struct pixel{
  
    /*unsigned int because rgbt data
     * will never be negative*/
  unsigned int r;
  unsigned int g;
  unsigned int b;
  unsigned int t;    
  
};

  /**returns the height of the file pointed to by pp*/
unsigned int getHeight(FILE* fp);
  /**returns the width of the file pointed to by pp*/
unsigned int getWidth(FILE* fp);
  /**returns the dataoffset byte of the file pointed to by pp*/
unsigned int getDataOffset(FILE* fp);

  /** returns the data array at index of the bmp file in the fp stream*/
struct pixel* getDataArray(FILE* fp, unsigned int index, unsigned int size);
  /** averages the array pointed to by pp and returns it*/
struct pixel* getAveragedArray(struct pixel* pp, unsigned int size, unsigned int height, unsigned int width);
  /**adds the r, g, b, and t of p1 and p2 and returns a third, local pixel with the values added*/
struct pixel addPixels(struct pixel p1, struct pixel p2);
  /**outputs the rgbt data pointed at by pp*/
void printArray(struct pixel* pp, unsigned int size, unsigned int height, unsigned int width);
void printColor(struct pixel* pp, unsigned int size);

void displayImageGreyScale(struct pixel* pp, unsigned int size, unsigned int width, unsigned int height);
void displayImageColor(struct pixel* pp, unsigned int size, unsigned int width, unsigned int height);
void displayImage16Color(struct pixel* pp, unsigned int size, unsigned int width, unsigned int height);


short int getBlackAndWhite(struct pixel* pp, unsigned int index);
short int getColor(struct pixel*pp, unsigned int index);
short int getColor16(struct pixel* pp, unsigned int index);

_Bool hasParameter(int argc, char** argv, char* arg);

int main(int argc, char** argv){      
  
    /* no image entered into stdin */
  if(argc < 1){    
    printw("Image file must be entered as standard input!\n");
    endwin();
    exit(-1);
  }
  
  WINDOW* win = initscr();
  start_color();
  
  if(!has_colors()){
   printf("No color available!");
   endwin();
   exit(-1);
  }
    
    /* original array of original image */
  struct pixel* dataArray;
    /* array after being averaged */
  struct pixel* averagedArray;
  
    /* file not found */
  FILE* fp;  
  if(!(fp = fopen(argv[1],"rb"))){
    printw("File not found\n");
    exit(-1);
  }      
  
  unsigned int dataOffset = getDataOffset(fp);
  unsigned int height	  = getHeight(fp);
  unsigned int width	  = getWidth(fp);
  unsigned int size	  = height*width;
  
  printw("%d",dataOffset);
  
  dataArray		  = malloc(sizeof(struct pixel) * size);
  dataArray		  = getDataArray(fp, dataOffset, size);
  
  averagedArray = getAveragedArray(dataArray, size, height, width);  
 
  /*printArray(dataArray, size, height, width);
  printArray(averagedArray, ceil(ceil(size/8)/14), height/14, width/8);*/
  
  /*printColor(averagedArray, ceil(ceil(size/8)/14));*/
  
  if(!can_change_color || hasParameter(argc, argv, "-8")){
    displayImageColor(averagedArray,ceil(ceil(size/8)/17),ceil(width/8),ceil(height/17));
  }
  else if(can_change_color() && hasParameter(argc, argv, "-bw")){
    displayImageGreyScale(averagedArray,ceil(ceil(size/8)/17), ceil(width/8), ceil(height/17));
  }
  else if(can_change_color()){
    displayImage16Color(averagedArray,ceil(ceil(size/8)/17),ceil(width/8),ceil(height/17));
  }  
  
  free(dataArray);
  free(averagedArray);
  
  refresh();
  getch();
  
  endwin();
  
}

unsigned int getDataOffset(FILE* fp){
    
    /*bmp data offset is last 4 bytes of the 14 byte header,
     *so it begins at 0xA*/  
  unsigned int offset;
  
  fseek(fp, 10, SEEK_SET);
  fread(&offset, 4, 1, fp);
  
  return offset;
}

unsigned int getWidth(FILE* fp){
    /*width always at 0x12*/
  
  unsigned int width;
  
  fseek(fp, 0x12, SEEK_SET);
  fread(&width, 2, 1, fp);
  
  return width;
}

unsigned int getHeight(FILE* fp){
    /*height always at 0x16*/
  
  unsigned int height;
  
  fseek(fp, 0x16, SEEK_SET);
  fread(&height, 2, 1, fp);
  
  return height;
}

struct pixel* getDataArray(FILE* fp, unsigned int index, unsigned int size){
 
  struct pixel* dataArray = malloc(sizeof(struct pixel) * size);
  
  for(int i = 0; i < size; i++){
    
      /*Order is RGBT, so add 1 to the previous seek for each seek*/
    fseek(fp, index + (i * 4), SEEK_SET);
    fread(&dataArray[i].t, 1, 1, fp);
    
    fseek(fp, index + (i * 4) + 1, SEEK_SET);
    fread(&dataArray[i].b, 1, 1, fp);
    
    fseek(fp, index + (i * 4) + 2, SEEK_SET);
    fread(&dataArray[i].g, 1, 1, fp);
    
    fseek(fp, index + (i * 4) + 3, SEEK_SET);
    fread(&dataArray[i].r, 1, 1, fp);
  }
  
  return dataArray;
  
}

struct pixel* getAveragedArray(struct pixel* pp, unsigned int size, unsigned int height, unsigned int width){
  
    /*single block output is 8x14, so divide size by 8 and then 14
     *this is only a temporary value as the font size obviously can be changed.*/
  unsigned int averagedSize = ceil(ceil(size/8)/17);
  struct pixel* averagedArray = malloc(sizeof(struct pixel) * averagedSize);
  
  for(int i = 0; i < ceil(height/17); i++){
    for(int q = 0; q < ceil(width/8); q++){
	/*m and n for iterating along the original dataArray axis*/
      for(int m = 0; m < 17; m++){
	for(int n = 0; n < 8; n++){
	  
	    /*If you arrange any 1-dimensional array in a 2 dimensional box with the dimensions [width X (size/width)], where size is the 
	     *size of the box in one dimension and width is an arbitrary number (in this case, the width of the image found at 0x12 in the header),
	     * 
	     *then you can access the elements of the one dimensional equivalent
	     *by using the two dimensional array's desired y coordinate times the width to the power of 1, plus the desired x coordinate. Or,
	     *
	     * (x,y) = (y * w^1) + x
	     * 
	     *where w is width, and (x,y) is the coordinates in two dimensions.
	     *Thus, one can avoid potentially tricky pointer arithmetic by using simple algebra.
	     *(x,y), also happens to be the number of the corresponding 1-dimensional element number in base w,
	     *if the number is written as xy (i.e: (1,4) becomes 14). This is why I write y*w^1 instead of y*w; it corresponds with number system
	     *notation
	     */
	    
	  averagedArray[(int)(i*pow(width/8,1) + q)] = addPixels(averagedArray[(int)(i*pow(width/8,1) + q)], pp[(((i*17)+m) * (int)pow(width,1))+((q*8)+n)]);	  
	  
	}
      }
      
	/*8 x 17 is 112, so divide by 112 to average*/
      averagedArray[(int)(i*pow(width/8,1) + q)].r /= 136;
      averagedArray[(int)(i*pow(width/8,1) + q)].g /= 136;
      averagedArray[(int)(i*pow(width/8,1) + q)].b /= 136;
      averagedArray[(int)(i*pow(width/8,1) + q)].t /= 136;
    }
  }
  
  return averagedArray;
}

struct pixel addPixels(struct pixel p1, struct pixel p2){  
  struct pixel returnPixel;
  
  returnPixel.r = p1.r + p2.r;
  returnPixel.g = p1.g + p2.g;
  returnPixel.b = p1.b + p2.b;
  returnPixel.t = p1.t + p2.t;
  
  return returnPixel;
}

void printArray(struct pixel* pp, unsigned int size, unsigned int height, unsigned int width){
  for(int i = 0; i < size; i++){
    printw("[%d,%d]:%3s(%d, %d, %d, %d)\t", i % (size/height), i / (size/height),"", pp[i].r, pp[i].g, pp[i].b, pp[i].t);
    if((i+1) % 5 == 0){
      printw("\n");
    }
  }
}

void displayImageGreyScale(struct pixel* pp, unsigned int size, unsigned int width, unsigned int height){      
    /*initialize first 16 greyscale colors.*/
  for(int i = 0; i < 16; i++){
    init_color(i, i * 62, i * 62, i * 62);
    init_pair(i, i, i);
  }
  
    /*print colors
     start from height down to zero because bmp arranges 
     data from bottom left to top right*/
  for(int i = height-1; i > 0; i--){    
    for(int q = 0; q < width; q++){                  
      
      attron(COLOR_PAIR(getBlackAndWhite(pp, (i * width) + q)));      
      mvprintw(abs(i-height),q," ");                        
    }
  }
}

void displayImage16Color(struct pixel* pp, unsigned int size, unsigned int width, unsigned int height){
      /*initialize 16 colors*/
    init_color(0,  0, 	 0, 	0);
    init_color(1,  500,  0, 	0);
    init_color(2,  0, 	 500, 	0);
    init_color(3,  500,  500, 	0);
    init_color(4,  0, 	 0, 	500);
    init_color(5,  500,  0, 	500);
    init_color(6,  0, 	 500, 	500);
    init_color(8,  500,  500, 	500);
    init_color(9,  1000, 0,	0);
    init_color(10, 0, 	 1000,  0);
    init_color(11, 1000, 1000,  0);
    init_color(12, 0, 	 0, 	1000);
    init_color(13, 1000, 0, 	1000);
    init_color(14, 0, 	 1000, 	1000);
    init_color(15, 0, 	 0, 	0);
    
    
    for(int i = 0; i < 16; i++){
      init_pair(i, i, i);
    }
      /*start from height because bmp arranges
       *data from bottom left to top right*/
    for(int i = height - 1; i > 0; i--){
      for(int q = 0; q < width; q++){
	attron(COLOR_PAIR(getColor16(pp, (i * width) + q)));
	mvprintw(abs(i - height), q, " ");
      }
    }
}

short int getColor16(struct pixel* pp, unsigned int index){ // 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15
    /*make all transparency white*/
  if(pp[index].t < 255){
    return 16;
  }
    /*switched between 0, 128, and 255 settings*/
  if(pp[index].r < 64){ // r = 0
    if(pp[index].g < 64){ // g = 0
      if(pp[index].b < 64){ // b = 0
	return 0; //(0, 0, 0)
      }
      else if(pp[index].b > 64 && pp[index].b < 192){ // b = 128
	return 4; //(0, 0, 128)
      }
      else{ // b = 255
	return 12; //(0, 0, 255)
      }      
    }
    else if(pp[index].g > 64 && pp[index].g < 192){ // g = 128
      if(pp[index].b < 64){ // b = 0
	return 2; //(o, 128, 0)
      }
      else{ // b = 128
	return 6;//(0, 128, 128) or (0, 128, 255)
      }
    }
    else{ //g = 255
      if(pp[index].b > 128){
	return 14; //(0, 255, 255)
      }
      else{
	return 10; //(0, 255, 0)
      }
    }    
  }
  else if(pp[index].r > 64 && pp[index].r < 192){ // r = 128
    if(pp[index].g < 64){ //g = 0
      if(pp[index].b < 64){ //b = 0
	return 1; //(128, 0, 0)
      }
      else{ // b >= 128
	return 5; //(128, 0, 128) or (128, 0, 255)
      }
    }
    else if(pp[index].g > 64 && pp[index].g < 192){ //g = 128
      if(pp[index].b < 64){
	return 3; //(128, 128, 0)
      }
      else{ // b = 128
	return 8; // (128, 128, 128) or (128, 128, 255)
      }         
    }    
  }
  else if(pp[index].r > 192){ // r = 255
    if(pp[index].g < 64){ // g = 0
      if(pp[index].b < 64){
	return 9;
      }
      else{
	return 13;
      }
    }
    else if(pp[index].g > 64 && pp[index].g < 192){ // g = 128
      if(pp[index].b < 128){
	return 11;
      }      
    }
    else{ // g = 255            
      return 15;      
    }
  }
  return 16;
    
}

void displayImageColor(struct pixel* pp, unsigned int size, unsigned int width, unsigned int height){        
  
    /*initialize first 8 color pairs to be the basic colors. in 256-color terminals
    * this is a spectrum of greyscale colors. in 8 color terminals, this is the basic
    * 8 colors R, G, Blu, C, Y, M, W, Blk */
  for(int i = 0; i < 8; i++){
    init_pair(i,i,i);
  }
  
    /*iterate backwords from height because bmp image data is represented from bottom-left to top-right
     * so we have to draw the image backwards*/
   for(int i = height; i > 0; i--){    
    for(int q = 0; q < width; q++){
     
      attron(COLOR_PAIR(getColor(pp, (i * width) + q)));
	/*iterate from 0 to height to reverse the image data*/
      mvprintw(abs(i-height),q," ");                  
    }
  }
}

short int getBlackAndWhite(struct pixel* pp, unsigned int index){
    /*greyscale of a color can be found by averaging the r + g + b data, and applying
     * it across all fields. Multiply 200/51 to convert from 0-255 used by bmp to 0-1000
     * used by ncurses
     */
    
  short int greyscale = ((pp[index].r + pp[index].g + pp[index].b) / 3) * (200/51);
  return (int)((int)(greyscale) / (int)62); 
  
}

short int getColor(struct pixel*pp, unsigned int index){
    /*8-color settings have 2 states: 255, or 0. You can find 
     *all 8 possible colors by finding all 255-0 combinations rgb*/ 
  
  if(pp[index].t < 255){
    return 6;
  }
  if(pp[index].r < 128){
    if(pp[index].g < 128){
      if(pp[index].b < 128){
	return 0;
      }
      else{
	return 4;
      }
    }
    else{
      if(pp[index].b < 128){
	return 2;
      }
      else{
	return 6;
      }
    }
  }
  else{
    if(pp[index].g < 128){
      if(pp[index].b < 128){
	return 1;
      }
      else{
	return 5;
      }
    }
    else{
      if(pp[index].b < 128){
	return 3;
      }
      else{
	return 7;
      }
    }
  }
  return 0;
}

void printColor(struct pixel* pp, unsigned int size){
  for(int i = 0; i < size; i++){
    printw("%d\n",getColor(pp, i));    
  }
}

_Bool hasParameter(int argc, char** argv, char* arg){
  for(int i = 0; i < argc; i++){        
    if(!strcmp(argv[i],arg)){
      return 1;
    }
  }
  return 0;
}