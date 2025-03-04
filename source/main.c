#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>

/*
Stuff for me to remember:
	//bottomScreen.fg = 0x07E0;  // sets color of text
	//bottomScreen.bg = 0x07E0;  // sets color of pixels behind text
Screen dimensions:
	-320 pixels width
	-240 pixels height
	-32 character width
Need to convert image to raw pixel data for 3ds (.binary)
File path for files ON the actual 3ds: "sdmc:/other/image.bin"

3DS has a 24-bit RGB palette
3DS supports RGB, Greyscale, Bitmap and Indexed Color
3DS supports SGI Image File format in -8 and -16 bit color depth
3DS is a LITTLE-endian system

3DS Bottom Screen: 320x240
3DS Top Screen   : 800x240
*/

//function to convert RGB values to RGB565
//<u8 r>	red component of color, (0-255), 8-bit unsigned int
//<u8 g>	green component of color, (0-255), 8-bit unsigned int
//<u8 b>0	blue component of color, (0-255), 8-bit unsigned int
//<Returns>	returns 16-bit converted rgb565 value
u32 rgbToRGB565(u8 r, u8 g, u8 b) {
	//convert each color channel to appropriate number of bits
	u32 red = (r >> 3) & 0x1F;		// 5 bits for red	/ right shift 3 and mask (11111)
	u32 green = (g >> 2) & 0x3F;	// 6 bits for green	/ right shift 2 and mask (111111)
	u32 blue = (b >> 3) & 0x1F;		// 5 bits for blue	/ right shift 3 and mask (11111)

	// combine RGB into single 16-bit rgb565 value
	u32 color565 = (red << 11) | (green << 5) | blue;
	return color565;// | 0x00000000;
}

//function used to print errors
//<message>		pointer to a string to be output
void printError(const char* message) 
{
	//prints a DESCRIPTION of the last error
    perror(message);
	//prints the standard error message
    printf("Error: %s\n", message);
}

//function to load images onto 3ds screen
//<filepath>		path to image to load (string/char)
//<framebuffer>		pointer to frame to write to 
//<width>			image to load width in pixels, int
//<height>			image to load height in pixels, int
void loadImage(const char* filePath, u32* framebuffer, int width, int height) 
{
	//attempt to open image in read binary mode
	FILE* imageFile = fopen(filePath, "rb");

	//print error if doesnt work
	if (!imageFile) 
	{
		printError("Error opening image file\n");
		return;
	}

	//allocate mem for image data (in RGB format: 3 bytes per pixel)
	size_t imageSize = width * height * 3; 
	u8* imageData = (u8*)malloc(imageSize);

	//print error if doesnt work and close image
	if (!imageData) 
	{
		printError("Memory allocation failed\n");
		fclose(imageFile);
		return;
	}

	//was going to be a test to see if the amount of bytes in the file matches up to the size, but it was not working
	// 
	//size_t bytesRead  = fread(imageData, 1, imageSize, imageFile);
	//make sure file size matches expected
	//if (bytesRead != imageSize)
	//{
	//	printf("File size mismatch: Expected %zu bytes, got %zu bytes\n", imageSize, bytesRead);
	//	free(imageData);
	//	return;
	//}

	//read the image data from the file, into memory
	fread(imageData, 1, imageSize, imageFile);
	//close file after reading
	fclose(imageFile);


	//offsets that were going to be used to center the image (never worked)
	//int xOffset = (320 - width) / 2;
	//int yOffset = (240 - height) / 2; 
	//framebuffer[(y + yOffset) * 320 + (x + xOffset)] = rgbToRGB565(r, g, b) & 0xFFFF;

	//loop through every single pixel in the image
	for (int y = 0; y < height; y++) 
	{
		for (int x = 0; x < width; x++) 
		{
			//for each pixel extract individual RGB components
			//image data is stored in row-major order with 3 bytes per pixel
			u8 r = imageData[(y * width + x) * 3 + 0]; // red component    byte1
			u8 g = imageData[(y * width + x) * 3 + 1]; // green component  byte2
			u8 b = imageData[(y * width + x) * 3 + 2]; // blue component   byte3

			//old debugging statements
			//printf("r: %u, g: %u, b: %u\n", r, g, b);
			//printf("Color: %x\n", rgbToRGB565(r, g, b));

			//convert from RGB to RGB565 (16-bit color format)
			//store converted color in framebuffer at position
			//& 0xFFFF to ensure we are only using the lower 16 bits (RGB565 format)
			framebuffer[y * 320 + x] = rgbToRGB565(r, g, b) & 0xFFFF;

		}
	}

	//free the allocated memory after processing is complete
	free(imageData);
}

//function sets the color of the top and bottom screens to a given color
//<u32 color>		color to switch to (32 bit unsigned int)
void setScreenColor(u32 color) 
{
	//get the frame buffer of the top screen
	u32* framebuffer = (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);  //top screen
	//loop thru every pixel in the buffer and set color
	//top resolution: 800x240
	for (int i = 0; i < 800 * 240; i++) 
	{
		framebuffer[i] = color;  
	}

	//get the frame buffer of the bottom screen
	framebuffer = (u32*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);  //bottom screen
	//loop thru every pixel in the buffer and set color
	//bottom resolution: 320x240
	for (int i = 0; i < 320 * 240; i++)
	{
		framebuffer[i] = color;
	}
}

//function prints given text to the center of the given screen
//<screen>		print console of where the words should be output
//<text>		pointer to string of words to be output
void printCenteredText(PrintConsole* screen, const char* text)
{
	//calculate length of the string
	int textLength = strlen(text);

	//calculate center of screen based off console width and height
	int startX = (screen->consoleWidth - textLength) / 2;
	int startY = (screen->consoleHeight - 1) / 2;

	//set the cursor position to the calculated points
	screen->cursorX = startX;
	screen->cursorY = startY;

	//print text here
	printf("%s", text);
}

//function to continuously draw a square where the users is touching
//<x>				x and y have been flopped (for whatever reason this is what worked correctly)
//<y>				y = touch.PX and x = touch.PY
//<size>			length of the square to be drawn
//<u32 color>		color of the square (32 unsigned int)
void drawSquare(int y, int x, int size, u32 color) 
{
	//get the frame buffer of the bottom screen
	//only the bottom screen has touchscreen capabilities
	u32* frameBuffer = (u32*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

	//loop through each pixel in the square 
	//dx horizontal offset, dy vertical offset
	for (int dx = 0; dx < size; dx++) 
	{
		for (int dy = 0; dy < size; dy++) 
		{
			//calculate index of pixel to be colored
			//(x + dx) = horizontal pos
			//(y + dy) = vertical pos
			//multiply by 240 to account for framerbuffers layout
			//divide by 2 to account for 16-bit color format
			frameBuffer[(((x + dx) + (y + dy) * 240) / 2)] = color; 
		}
	}
}

//main function
//holds "gameplay" loop
int main(int argc, char* argv[])
{
	//init graphics system
	gfxInitDefault();

	//create print consoles for both screens
	PrintConsole topScreen, bottomScreen;
	//create touchposition for bottom screen
	touchPosition touch;

	//init both screens for console output
	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);

	//define colors to be used
	u32 bgColorGreen = 0x07E0;
	u32 bgColorRed = 0xF800;
	u32 bgColorGrey = 0x7777;

	//set up starting screen
	//start with red screen and basic words
	setScreenColor(bgColorRed);
	consoleSelect(&topScreen);		//choose top screen and print
	printCenteredText(&topScreen, "Top Screen Up here!");
	consoleSelect(&bottomScreen);	//choose bottom screen and print
	printCenteredText(&bottomScreen, "Hello bottom screen :)!");

	//define path to image to be loaded
	const char* imagePath = "sdmc:/3ds/Application/assets/link.raw";

	//past file fails (they were not formatted correctly)
	//const char* imagePath = "sdmc:/application/assets/spriteLink.raw";
	//const char* imagePath = "sdmc:/application/assets/linkTWO.raw";
	//const char* imagePath = "sdmc:/application/assets/solidRed.raw";
	//const char* imagePath = "sdmc:/application/assets/solidRed2.raw";
	//const char* imagePath = "sdmc:/application/assets/linkWorking.raw";
	//const char* imagePath = "sdmc:/application/assets/linkTWONoColor.raw";
	//const char* imagePath = "sdmc:/application/assets/linkThree.raw";
	//const char* imagePath = "sdmc:/application/assets/linkThree.sgi";
	//loadImageRAW(imagePath, (u32*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL), 200, 200, 3); 

	//flags to control functionality
	bool inStrobeMode = false;
	bool isGreen = true;
	int tickCount = 0;
	u32 prevKeysDown = 0;

	//main loop runs until user shuts down
	while (aptMainLoop())
	{
		gspWaitForVBlank();			//
		gfxSwapBuffers();			//swap buffers to update screen
		hidScanInput();				//scan for input events
		tickCount++;				//increment tick count each frame
		u32 kDown = hidKeysDown();	//get keys pressed this frame

		//SELECT TOGGLE FUNCTIONALITY (switching strobe mode on and off)
		//prevent toggling in the same frame
		if ((kDown & KEY_SELECT) && !inStrobeMode && !(prevKeysDown & KEY_SELECT)) 
		{
			//print informative text
			printCenteredText(&topScreen, "Strobe Mode On\n");
			inStrobeMode = true;
		}
		else if ((kDown & KEY_SELECT) && inStrobeMode && !(prevKeysDown & KEY_SELECT))
		{
			//print informative text
			printCenteredText(&topScreen, "Strobe Mode Off\n");
			inStrobeMode = false;
		}

		//STROBE FUNCTIONALITY
		if (inStrobeMode)
		{
			//if in strobe mnode, change color every 100 ticks
			//switching between red and green
			if (tickCount % 100 == 0)
			{
				//toggle color
				isGreen = !isGreen;
				//apply new color and some text
				setScreenColor(isGreen ? bgColorGreen : bgColorRed);
				consoleSelect(&topScreen);
				printCenteredText(&topScreen, isGreen ? "In Green" : "In Red");
				gfxSwapBuffers();
			}
		}

		//BUTTON FUNCTIONALITY
		//B BUTTON: to clear the screen
		if (kDown & KEY_B)
		{
			//set both screens to grey, and output a success message
			setScreenColor(bgColorGrey);
			consoleSelect(&bottomScreen);
			printCenteredText(&bottomScreen, "Successfully! :)");
			consoleSelect(&topScreen);
			printCenteredText(&topScreen, "Screen Cleaned");
		}
		//START BUTTON: to close the program
		else if (kDown & KEY_START)
		{
			break;
		}
		//A BUTTON: loads an image of link on the bottom screen
		else if (kDown & KEY_A)
		{
			svcSleepThread(3000);
			loadImage(imagePath, (u32*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL), 320, 240);
		}

		//TOUCH SCREEN FUNCTIONALITY
		//first read touch input
		hidTouchRead(&touch);

		//if touched position is within bounds...
		if ((touch.px >= 0) && (touch.px <= 320) && (touch.py >= 0) && (touch.py <= 240)) 
		{
			//draw a square at the position (square draws a line if user holds down)
			//invert the Y-axis for proper drawing
			int correctedY = 240 - touch.py;  
			drawSquare(touch.px, correctedY, 10, RGB565(255, 255, 255));
		}

		//save the keys from this frame, so they can be used next frame
		prevKeysDown = kDown;
	}

	//exit graphics system and clean up
	gfxExit();
	return 0;
}
