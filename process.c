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



typedef struct img_t
{
	uint8_t * buf;
	int width;
	int height;
} img_t;


typedef struct border_find_t
{
	uint8_t * start_pnt;
	int sum_dv;
	int cnt_dv;
	int min_dv;
	int wall;
	
} border_find_t;



static int mv_border( img_t * img, border_find_t * bf, int x, int y )
{
	uint8_t * c_pnt = img->buf + 3 * ( ( img->height - y ) * img->width + x );
	int dr = ( *( bf->start_pnt ) - *c_pnt );
	int dg = ( *( bf->start_pnt + 1 ) - *( c_pnt + 1 ) );
	int db = ( *( bf->start_pnt + 2 ) - *( c_pnt + 2 ) );
	int dv = dr * dr + dg * dg + db * db + bf->min_dv;

	if( bf->cnt_dv )
	{
		/* а не граница ли тут */
		if( dv * bf->cnt_dv > bf->sum_dv * bf->wall )
		{
			return 1; /* есть граница */
		}
	}

	bf->cnt_dv++;
	bf->sum_dv += dv;

	return 0;
}


#define RAY_LEN 100

static void one_X( img_t * img, int cx, int cy, float ang )
{
	border_find_t bf_data;
	bf_data.start_pnt = img->buf + 3 * ( ( img->height - cy ) * img->width + cx );
	bf_data.sum_dv = 0;
	bf_data.cnt_dv = 0;
	bf_data.wall = 22; /* установлено шаманами */
	bf_data.min_dv = 14; /* определено шаманами */

	float st_x = cos( ang );
	float st_y = sin( ang );

	/* Нарисуем оси */
        glBegin( GL_LINES );
        glColor3f( 0.3, 0.3, 0.3 );
        glVertex3f( cx - ( int )( RAY_LEN * st_x ), cy + ( int )( RAY_LEN * st_y ), 0 );
        glVertex3f( cx + ( int )( RAY_LEN * st_x ), cy - ( int )( RAY_LEN * st_y ), 0 );
        glVertex3f( cx - ( int )( RAY_LEN * st_y ), cy - ( int )( RAY_LEN * st_x ), 0 );
        glVertex3f( cx + ( int )( RAY_LEN * st_y ), cy + ( int )( RAY_LEN * st_x ), 0 );
        glEnd();


	int i;

	/* справа */
	for( i = 1; i < RAY_LEN; i++ )
	{
		int ix = cx + ( int )( i * st_x );
		int iy = cy - ( int )( i * st_y );

		if( ( ix >= img->width ) || ( ix < 0 ) ) break;
		if( ( iy >= img->height ) || ( iy < 0 ) ) break;

		if( mv_border( img, &bf_data, ix, iy ) )
		{
			glBegin( GL_LINES );
			glColor3f( 0.4, 1, 0.4 );
			glVertex3f( ix - 2, iy - 2, 0);
			glVertex3f( ix + 2, iy + 2, 0);
			glVertex3f( ix - 2, iy + 2, 0);
			glVertex3f( ix + 2, iy - 2, 0);
			glEnd();
			break;
		}
	}

	/* слева */
	for( i = 1; i < RAY_LEN; i++ )
	{
		int ix = cx - ( int )( i * st_x );
		int iy = cy + ( int )( i * st_y );

		if( ( ix >= img->width ) || ( ix < 0 ) ) break;
		if( ( iy >= img->height ) || ( iy < 0 ) ) break;

		if( mv_border( img, &bf_data, ix, iy ) )
		{
			glBegin( GL_LINES );
			glColor3f( 1, 1, 1 );
			glVertex3f( ix - 2, iy - 2, 0);
			glVertex3f( ix + 2, iy + 2, 0);
			glVertex3f( ix - 2, iy + 2, 0);
			glVertex3f( ix + 2, iy - 2, 0);
			glEnd();
			break;
		}
	}

	/* сверху */
	for( i = 1; i < RAY_LEN; i++ )
	{
		int ix = cx - ( int )( i * st_y );
		int iy = cy - ( int )( i * st_x );

		if( ( ix >= img->width ) || ( ix < 0 ) ) break;
		if( ( iy >= img->height ) || ( iy < 0 ) ) break;

		if( mv_border( img, &bf_data, ix, iy ) )
		{
			glBegin( GL_LINES );
			glColor3f( 1, 1, 1 );
			glVertex3f( ix - 2, iy - 2, 0);
			glVertex3f( ix + 2, iy + 2, 0);
			glVertex3f( ix - 2, iy + 2, 0);
			glVertex3f( ix + 2, iy - 2, 0);
			glEnd();
			break;
		}
	}


	/* снизу */
	for( i = 1; i < RAY_LEN; i++ )
	{
		int ix = cx + ( int )( i * st_y );
		int iy = cy + ( int )( i * st_x );

		if( ( ix >= img->width ) || ( ix < 0 ) ) break;
		if( ( iy >= img->height ) || ( iy < 0 ) ) break;

		if( mv_border( img, &bf_data, ix, iy ) )
		{
			glBegin( GL_LINES );
			glColor3f( 1, 1, 1 );
			glVertex3f( ix - 2, iy - 2, 0);
			glVertex3f( ix + 2, iy + 2, 0);
			glVertex3f( ix - 2, iy + 2, 0);
			glVertex3f( ix + 2, iy - 2, 0);
			glEnd();
			break;
		}
	}
}

void process_rgb_frame( uint8_t *img, int img_width, int img_height )
{
	img_t frame;
	frame.buf = img;
	frame.width = img_width;
	frame.height = img_height;


	int i;
	for( i = 0; i < 100; i++ )
	{	
		one_X( &frame, img_width / 2, img_height / 2, (float)( rand() % 1000 ) * 2 * M_PI / 1000.0 );
	}
}

