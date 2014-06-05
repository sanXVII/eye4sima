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
	int dr = /*( *c_pnt > *( bf->start_pnt ) ) ? 
		( *c_pnt - *( bf->start_pnt ) ) :*/ ( *( bf->start_pnt ) - *c_pnt );

	int dg = /*( *( c_pnt + 1 ) > *( bf->start_pnt + 1 ) ) ?
		( *( c_pnt + 1 ) - *( bf->start_pnt + 1 ) ) : */
		( *( bf->start_pnt + 1 ) - *( c_pnt + 1 ) );

	int db = /*( *( c_pnt + 2 ) > *( bf->start_pnt + 2 ) ) ?
		( *( c_pnt + 2 ) - *( bf->start_pnt + 2 ) ) : */
		( *( bf->start_pnt + 2 ) - *( c_pnt + 2 ) );

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

static void mk_horizont_border( img_t * img, int cx, int cy )
{
	border_find_t bf_data;
	bf_data.start_pnt = img->buf + 3 * ( ( img->height - cy ) * img->width + cx );
	bf_data.sum_dv = 0;
	bf_data.cnt_dv = 0;
	bf_data.wall = 22; /* установлено шаманами */
	bf_data.min_dv = 14; /* определено шаманами */

	/* Нарисуем оси */
        glBegin( GL_LINES );
        glColor3f(1, 1, 1);
        glVertex3f( cx - RAY_LEN, cy, 0);
        glVertex3f( cx + RAY_LEN, cy, 0);
        glVertex3f( cx, cy - 2, 0);
        glVertex3f( cx, cy + 2, 0);
        glEnd();


	int dx;

	/* справа */
	for( dx = 1; ( dx < 100 ) && ( cx + dx < img->width ); dx++ )
	{
		if( mv_border( img, &bf_data, cx + dx, cy ) )
		{
			glBegin( GL_LINES );
			glColor3f( 0.8, 1, 0.8 );
			glVertex3f( cx + dx, cy - 2, 0);
			glVertex3f( cx + dx, cy + 2, 0);
			glEnd();
			break;
		}
	}

	/* слева */
	for( dx = 1; ( dx < 100 ) && ( cx - dx >= 0 ); dx++ )
	{
		if( mv_border( img, &bf_data, cx - dx, cy ) )
		{
			glBegin( GL_LINES );
			glColor3f( 0.8, 1, 0.8 );
			glVertex3f( cx - dx, cy - 2, 0);
			glVertex3f( cx - dx, cy + 2, 0);
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

	

	/* Тут мы будем искать все наши маркеры.
	 * Также можно отрисовать через opengl */
	int a;
	for( a = 2; a < 80; a++ )
	{
		mk_horizont_border( &frame, img_width / 2, 10 + a * 5 );
	}
}

