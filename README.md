This program was written by Michael Isenhour on Saturday November 22nd, 2025

Solid Image Background Remover (C Program)

This is a small but powerful C program that removes a solid-color from a background image and outputs a clean PNG with transparency. I created this as a personal challenge as well as a learning experience, and because I wanted a way to remove background color from some AI-generated sprite sheets I created.

This tool can: 
Remove a specific color that is given as a hex code like FF00FF
Automatically detect the background color (—auto)
Clean up fringes around the sprite
Output proper transparent PNG using stb_image and stb_image_write

Why I Built This:
As stated above I built this as a personal project because I needed to clean up some sprite sheets that had a flat pink/purple background and everything that I tried kept leaving tiny traces of leftover color around the edges. I also wanted something that was fast and simple and really just wanted to play around in C. Things I learned along the way were how to channel layouts and manipulate alpha. I also learned how to compute color distance in RGB space and how to carry out multi-pass image processing. 

Features:
Manual Color Removal — Tell the program exactly which color to remove using hex: 
./imgtool image.png output.png FF00FF

Auto Color Removal — Lets the program detect the background by sampling corners & center of image. 
./imgtool image.png output.png --auto

This works by averaging several obvious background pixels to find the real background color.


This program also features a two-pass cleaning where on it’s first pass it nukes everything close to the chosen background color and on it’s second pass it looks for halo pixels near transparency and removes them if they’re color adjacent to the background. After changing the background to transparent it then saves the file as a PNG file.

Building the Program
To use this program you only need a standard C compiler. You can compile this program with 
gcc -o imgtool main.c

IMPORTANT: make sure stb_image.h and stb_image_write.h are in the same folder as main.c


Usage Examples: 

Type ./imgtool  followed by the location of the image you want to change and then the destination you want the saved finished file finally followed by the color hex you want to be transparent.

To remove pink (magenta) background:
./imgtool sprite.png cleaned.png FF00FF

To auto-remove background:
./imgtool portrait.jpg transparent.png —auto

To remove green screen:
./imgtool footage.png output.png 00FF00

Dependencies
This project uses:
stb_image.h
stb_image_write.h
Both are public-domain single-header libs.
You can grab them here: https://github.com/nothings/stb

Final Thoughts:
This project reminded me of why C is such a fun language where nothing happens unless you make it happen. Being able. To build something that loads a real image processes thousands of pixels per second and then creates a fully transparent PNG…and works is absolutely amazing. If you want to download this tool and extend its capabilities please feel free to do so. 

