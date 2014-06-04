#include <GL/gl.h>
#include <GL/glu.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>


void process_rgb_frame( uint8_t *img, int img_width, int img_height )
{
	/* Тут мы будем искать все наши маркеры.
	 * Также можно отрисовать через opengl */

        glBegin( GL_LINES );
        glColor3f(1, 0, 0);
        glVertex3f(0, 0, 0);
        glVertex3f(500, 500, 0);
        glVertex3f(500, 0, 0);
        glVertex3f(0, 500, 0);
        glEnd();

}

