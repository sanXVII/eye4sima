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

	int result;

} border_find_t;



static int mv_border( img_t * img, border_find_t * bf, int x, int y, int way )
{
	if( ( x >= img->width ) || ( x < 0 ) ) return -1;
	if( ( y >= img->height ) || ( y < 0 ) ) return -1;

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
			glBegin( GL_LINES );
			if( way == 0x01 ) glColor3f( 0.4, 1, 0.4 ); else glColor3f( 1, 0.4, 0.4 );
			glVertex3f( x - 2, y - 2, 0);
			glVertex3f( x + 2, y + 2, 0);
			glVertex3f( x - 2, y + 2, 0);
			glVertex3f( x + 2, y - 2, 0);
			glEnd();

			bf->result |= way;

			return 1; /* есть граница */
		}
	}

	bf->cnt_dv++;
	bf->sum_dv += dv;

	return 0;
}


#define RAY_LEN 100

static int one_X( img_t * img, int * px, int * py, float ang )
{
	int cx = *px;
	int cy = *py;

	border_find_t bf_data;
	bf_data.start_pnt = img->buf + 3 * ( ( img->height - cy ) * img->width + cx );
	bf_data.sum_dv = 0;
	bf_data.cnt_dv = 0;
	bf_data.wall = 22; /* установлено шаманами */
	bf_data.min_dv = 14; /* определено шаманами */
	bf_data.result = 0;

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
	int ix;
	int iy;
	int ret_x = 0;
	int ret_y = 0;

	/* справа */
	for( i = 1; i < RAY_LEN; i++ )
	{
		ix = cx + ( int )( i * st_x );
		iy = cy - ( int )( i * st_y );

		if( mv_border( img, &bf_data, ix, iy, 0x01/* право */ ) ) break;
	}
	ret_x += ix;
	ret_y += iy;

	/* слева */
	for( i = 1; i < RAY_LEN; i++ )
	{
		ix = cx - ( int )( i * st_x );
		iy = cy + ( int )( i * st_y );

		if( mv_border( img, &bf_data, ix, iy, 0x02/* лево */ ) ) break;
	}
	ret_x += ix;
	ret_y += iy;

	/* сверху */
	for( i = 1; i < RAY_LEN; i++ )
	{
		ix = cx - ( int )( i * st_y );
		iy = cy - ( int )( i * st_x );

		if( mv_border( img, &bf_data, ix, iy, 0x04/* вверх */ ) ) break;
	}
	ret_x += ix;
	ret_y += iy;


	/* снизу */
	for( i = 1; i < RAY_LEN; i++ )
	{
		ix = cx + ( int )( i * st_y );
		iy = cy + ( int )( i * st_x );

		if( mv_border( img, &bf_data, ix, iy, 0x08/* вниз */ ) ) break;
	}
	ret_x += ix;
	ret_y += iy;

	if( bf_data.result == 0x0f/* все границы */ )
	{
		*px = ret_x / 4;
		*py = ret_y / 4;
		return 1; 
	}
	return 0;
}

void process_rgb_frame( uint8_t *img, int img_width, int img_height )
{
	img_t frame;
	frame.buf = img;
	frame.width = img_width;
	frame.height = img_height;


	int i;
	static int x = 300;//img_width / 2;
	static int y = 300;//img_height / 2;
	for( i = 0; i < 100; i++ )
	{
		one_X( &frame, &x, &y, (float)( rand() % 1000 ) * 2 * M_PI / 1000.0 );
	}
}

