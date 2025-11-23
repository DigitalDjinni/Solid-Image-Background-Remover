#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_image.h"
#include "stb_image_write.h"

// ------------------------------------------------------------
// Take one hex digit and turn it into a number 0–15.
// Basically: 'F' means 15, 'A' means 10, '7' means 7, etc.
// ------------------------------------------------------------
int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return -1; // not valid, throw it out
}

// ------------------------------------------------------------
// Takes a full hex color, like "FF00FF" or "#FF00FF", and turns
// it into normal R, G, B (0–255). If the user typed garbage,
// we send back an error so main() can quit early.
// ------------------------------------------------------------
int parse_hex_color(const char *hex, int *red, int *green, int *blue) {
    // If it starts with #, just skip it. Nobody cares.
    if (hex[0] == '#') hex++;

    // Needs to be exactly 6 characters or nope
    if (strlen(hex) != 6) return 1;

    int digits[6];
    for (int i = 0; i < 6; i++) {
        int v = hex_char_to_int(hex[i]);
        if (v < 0) return 1; // bad character
        digits[i] = v;
    }

    *red   = digits[0] * 16 + digits[1];
    *green = digits[2] * 16 + digits[3];
    *blue  = digits[4] * 16 + digits[5];
    return 0;
}

int main(int arg_count, char *arg_values[]) {

    // ------------------------------------------------------------
    // Make sure the user actually gave us:
    //   input file, output file, and hex color OR --auto
    // ------------------------------------------------------------
    if (arg_count < 4) {
        printf("Usage: %s <input_image> <output.png> <hex_color | --auto>\n", arg_values[0]);
        printf("Example with manual color: %s \"sprite.png\" output.png FF00FF\n", arg_values[0]);
        printf("Example with auto detect:  %s \"sprite.png\" output.png --auto\n", arg_values[0]);
        return 1;
    }

    const char *input_path  = arg_values[1];
    const char *output_path = arg_values[2];
    const char *hex_color   = arg_values[3];

    // --------------------------------------------------------------------
    // figure out the color we're supposed to delete
    // if user types a real hex code, we use that
    // if they type --auto or -a, we'll detect it later after loading image
    // --------------------------------------------------------------------
    int key_r = 0, key_g = 0, key_b = 0;
    int auto_mode = 0;

    if (strcmp(hex_color, "--auto") == 0 || strcmp(hex_color, "-a") == 0) {
        auto_mode = 1;
        printf("Auto mode activated — I’ll sniff the background myself.\n");
    } else {
        if (parse_hex_color(hex_color, &key_r, &key_g, &key_b) != 0) {
            printf("Bro what is this color?? %s\n", hex_color);
            return 1;
        }
        printf("Trying to erase this color: R=%d G=%d B=%d\n", key_r, key_g, key_b);
    }

    // ------------------------------------------------------------
    // Load the image as RGBA (even if the original didn't have alpha)
    // This gives us 4 bytes per pixel every time and it's super clean.
    // ------------------------------------------------------------
    int width, height, og_channels;
    unsigned char *img = stbi_load(input_path, &width, &height, &og_channels, 4);
    if (!img) {
        printf("Could not load image: %s\n", input_path);
        return 1;
    }

    printf("Loaded image %s (%dx%d, channels in file: %d)\n",
           input_path, width, height, og_channels);

    // Just for curiosity/debugging: peek at the top-left pixel
    unsigned char bg_r = img[0];
    unsigned char bg_g = img[1];
    unsigned char bg_b = img[2];
    printf("Top-left pixel color looks like: R=%d G=%d B=%d\n", bg_r, bg_g, bg_b);

    // --------------------------------------------------------------------
    // If we're in auto mode, we now pick the background color by sampling
    // some "obvious background" spots and averaging them.
    // --------------------------------------------------------------------
    if (auto_mode) {
        long sum_r = 0;
        long sum_g = 0;
        long sum_b = 0;
        int sample_count = 0;

        int coords[][2] = {
            {0, 0},
            {width - 1, 0},
            {0, height - 1},
            {width - 1, height - 1},
            {width / 2, 0},
            {width / 2, height - 1},
            {0, height / 2},
            {width - 1, height / 2},
            {width / 2, height / 2}
        };
        int num_coords = sizeof(coords) / sizeof(coords[0]);

        for (int i = 0; i < num_coords; i++) {
            int x = coords[i][0];
            int y = coords[i][1];

            int idx = (y * width + x) * 4;
            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];

            sum_r += r;
            sum_g += g;
            sum_b += b;
            sample_count++;
        }

        if (sample_count > 0) {
            key_r = (int)(sum_r / sample_count);
            key_g = (int)(sum_g / sample_count);
            key_b = (int)(sum_b / sample_count);
        }

        printf("Auto-detected background color: R=%d G=%d B=%d\n", key_r, key_g, key_b);
    }

    int total = width * height;

    // ------------------------------------------------------------
    // PASS 1: Basic “remove anything close to the chosen color”
    //
    // Think of this like the first swing of a lawnmower:
    // It chops down the big obvious background pixels that match
    // the chosen color pretty closely.
    // ------------------------------------------------------------
    int tolerance1 = 30; // lower = stricter, higher = sloppier
    int tol1_sq = tolerance1 * tolerance1;
    int removed_pass1 = 0;

    for (int i = 0; i < total; i++) {
        int idx = i * 4;

        unsigned char r = img[idx + 0];
        unsigned char g = img[idx + 1];
        unsigned char b = img[idx + 2];

        // distance between two colors in RGB space
        int dr = (int)r - key_r;
        int dg = (int)g - key_g;
        int db = (int)b - key_b;

        int dist_sq = dr*dr + dg*dg + db*db;

        // If it's close enough, nuke it
        if (dist_sq <= tol1_sq) {
            img[idx + 3] = 0; // alpha = 0 = invisible
            removed_pass1++;
        }
    }

    printf("Pass 1 removed %d pixels.\n", removed_pass1);

    // ------------------------------------------------------------
    // PASS 2: Halo cleanup
    //
    // This is where we go after the annoying leftover outline
    // pixels that aren't *quite* the target color, but are close,
    // AND they're sitting next to transparent pixels.
    //
    // Basically: "yo you look like leftover background, be gone."
    // ------------------------------------------------------------
    int tolerance2 = 120; // looser than pass 1
    int tol2_sq = tolerance2 * tolerance2;
    int removed_pass2 = 0;

    // Copy the image so we don’t screw up neighbor checks mid-loop
    unsigned char *cleaned = malloc(width * height * 4);
    if (!cleaned) {
        printf("Could not allocate memory for cleaned image.\n");
        stbi_image_free(img);
        return 1;
    }
    memcpy(cleaned, img, width * height * 4);

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {

            int idx = (y * width + x) * 4;

            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];
            unsigned char a = img[idx + 3];

            // skip stuff that's already gone
            if (a == 0) continue;

            // see how close this pixel is to the key color
            int dr = (int)r - key_r;
            int dg = (int)g - key_g;
            int db = (int)b - key_b;
            int dist_sq = dr*dr + dg*dg + db*db;

            // if it's not even *close* to the target color, leave it alone
            if (dist_sq > tol2_sq) continue;

            // check neighbors — if ANY neighbor is transparent,
            // this pixel is probably fringe and also needs to go
            int next_to_transparent = 0;

            for (int ny = -1; ny <= 1 && !next_to_transparent; ny++) {
                for (int nx = -1; nx <= 1; nx++) {
                    if (nx == 0 && ny == 0) continue;
                    int nidx = ((y + ny) * width + (x + nx)) * 4;
                    if (img[nidx + 3] == 0) {
                        next_to_transparent = 1;
                        break;
                    }
                }
            }

            if (next_to_transparent) {
                cleaned[idx + 3] = 0; // make this pixel transparent too
                removed_pass2++;
            }
        }
    }

    printf("Pass 2 cleaned up %d fringe pixels.\n", removed_pass2);

    // Move cleaned results back into img
    memcpy(img, cleaned, width * height * 4);
    free(cleaned);

    // ------------------------------------------------------------
    // Save the final PNG with transparency
    // ------------------------------------------------------------
    if (!stbi_write_png(output_path, width, height, 4, img, width * 4)) {
        printf("Could not save output: %s\n", output_path);
        stbi_image_free(img);
        return 1;
    }

    printf("All done! Saved output to %s\n", output_path);

    stbi_image_free(img);
    return 0;
}
