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
	if( ( x >= img->width ) || ( x < 0 ) ) return 0;
	if( ( y >= img->height ) || ( y < 0 ) ) return 0;

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
#define X_CYCLE_CNT 90
typedef struct maybe_figure
{
	int enter_x;
	int enter_y;

	float center_x;
	float center_y;
	int type; /* 0-unknown. 1-ellipse. 2-parallelogram */

	/* Temporal dataset */
	int point_Xs[ X_CYCLE_CNT * 4 ];
	int point_Ys[ X_CYCLE_CNT * 4 ];
	int point_cnt;

} maybe_figure;



static int one_X( img_t * img, maybe_figure * fig, float ang )
{
	int cx = fig->enter_x;
	int cy = fig->enter_y;

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
        //glBegin( GL_LINES );
        //glColor3f( 0.3, 0.3, 0.3 );
        //glVertex3f( cx - ( int )( RAY_LEN * st_x ), cy + ( int )( RAY_LEN * st_y ), 0 );
        //glVertex3f( cx + ( int )( RAY_LEN * st_x ), cy - ( int )( RAY_LEN * st_y ), 0 );
        //glVertex3f( cx - ( int )( RAY_LEN * st_y ), cy - ( int )( RAY_LEN * st_x ), 0 );
        //glVertex3f( cx + ( int )( RAY_LEN * st_y ), cy + ( int )( RAY_LEN * st_x ), 0 );
        //glEnd();


	int i;
	int ix;
	int iy;

	/* Центр одного последнего крестика */
	int ret_x = 0;
	int ret_y = 0;

	/* справа */
	for( i = 1; i < RAY_LEN; i++ )
	{
		ix = cx + ( int )( i * st_x );
		iy = cy - ( int )( i * st_y );

		if( mv_border( img, &bf_data, ix, iy, 0x01/* право */ ) ) break;
	}
	if( i == RAY_LEN ) return 0; /* Abort */

	ret_x += ix;
	ret_y += iy;

	fig->point_Xs[ fig->point_cnt ] = ix;
	fig->point_Ys[ fig->point_cnt ] = iy;
	fig->point_cnt++;

	/* слева */
	for( i = 1; i < RAY_LEN; i++ )
	{
		ix = cx - ( int )( i * st_x );
		iy = cy + ( int )( i * st_y );

		if( mv_border( img, &bf_data, ix, iy, 0x02/* лево */ ) ) break;
	}
	if( i == RAY_LEN ) return 0; /* Abort */

	ret_x += ix;
	ret_y += iy;

	fig->point_Xs[ fig->point_cnt ] = ix;
	fig->point_Ys[ fig->point_cnt ] = iy;
	fig->point_cnt++;

	/* сверху */
	for( i = 1; i < RAY_LEN; i++ )
	{
		ix = cx - ( int )( i * st_y );
		iy = cy - ( int )( i * st_x );

		if( mv_border( img, &bf_data, ix, iy, 0x04/* вверх */ ) ) break;
	}
	if( i == RAY_LEN ) return 0; /* Abort */

	ret_x += ix;
	ret_y += iy;

	fig->point_Xs[ fig->point_cnt ] = ix;
	fig->point_Ys[ fig->point_cnt ] = iy;
	fig->point_cnt++;

	/* снизу */
	for( i = 1; i < RAY_LEN; i++ )
	{
		ix = cx + ( int )( i * st_y );
		iy = cy + ( int )( i * st_x );

		if( mv_border( img, &bf_data, ix, iy, 0x08/* вниз */ ) ) break;
	}
	if( i == RAY_LEN ) return 0; /* Abort */

	ret_x += ix;
	ret_y += iy;

	fig->point_Xs[ fig->point_cnt ] = ix;
	fig->point_Ys[ fig->point_cnt ] = iy;
	fig->point_cnt++;

	if( bf_data.result == 0x0f/* все границы */ )
	{
		fig->enter_x = ret_x / 4;
		fig->enter_y = ret_y / 4;
		return 1; 
	}
	return 0;
}



int X_cycle( img_t * frame, maybe_figure * fig )
{
	/* Сбросим точки края старой фигурки .. будем искать новую */
	fig->point_cnt = 0;

	int ii;
	for( ii = 0; ii < X_CYCLE_CNT; ii++ )
	{
		if( !one_X( frame, fig, (float)( rand() % 1000 ) * 2 * M_PI / 1000.0 ) ) 
			break;
	}

	if( ii < ( X_CYCLE_CNT / 3 ) )
		return 0; /* A figure is not detected. */

	/* Найти центр */
	int i;
	fig->center_x = 0.0;
	fig->center_y = 0.0;
	for( i = 0; i < fig->point_cnt; i++ )
	{
		fig->center_x += fig->point_Xs[ i ];
		fig->center_y += fig->point_Ys[ i ];
	}
	fig->center_x /= ( float )i;
	fig->center_y /= ( float )i;

	return 1;
}



void process_rgb_frame( uint8_t *img, int img_width, int img_height )
{
	img_t frame;
	frame.buf = img;
	frame.width = img_width;
	frame.height = img_height;


	int i;
	for( i = 0; i < 1000; i++ )
	{
		maybe_figure fig;
		fig.enter_x = img_width * ( rand() % 2000 ) / 2000;
		fig.enter_y = img_height * ( rand() % 2000 ) / 2000;
		fig.type = 0/* unknown figure */;

		if( X_cycle( &frame, &fig ) )
		{
        		glBegin( GL_LINE_LOOP );
		        glColor3f( 1, 1, 1 );
		        glVertex3f( fig.center_x - 10, fig.center_y - 10, 0 );
		        glVertex3f( fig.center_x - 10, fig.center_y + 10, 0 );
		        glVertex3f( fig.center_x + 10, fig.center_y + 10, 0 );
		        glVertex3f( fig.center_x + 10, fig.center_y - 10, 0 );
		        glEnd();
		}
	}
}

