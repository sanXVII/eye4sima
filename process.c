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

	int start_x;
	int start_y;

	int x;
	int y;

	int new_center_x;
	int new_center_y;

} border_find_t;

#define RAY_LEN 100
#define X_CYCLE_CNT 100
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


static int mv_border( img_t * img, border_find_t * bf, maybe_figure * fig, float st_x, float st_y )
{
	int i;

	for( i = 1; i < RAY_LEN; i++ )
	{
		bf->x = bf->start_x + ( int )( i * st_x ); //------------------
		bf->y = bf->start_y - ( int )( i * st_y ); //------------------


		if( ( bf->x >= img->width ) || ( bf->x < 0 ) ) return 0;
		if( ( bf->y >= img->height ) || ( bf->y < 0 ) ) return 0;

		uint8_t * c_pnt = img->buf + 3 * ( ( img->height - bf->y ) * img->width + bf->x );
		int dr = ( *( bf->start_pnt ) - *c_pnt );
		int dg = ( *( bf->start_pnt + 1 ) - *( c_pnt + 1 ) );
		int db = ( *( bf->start_pnt + 2 ) - *( c_pnt + 2 ) );
		int dv = dr * dr + dg * dg + db * db + bf->min_dv;

		if( bf->cnt_dv )
		{
			/* а не граница ли тут */
			if( dv * bf->cnt_dv > bf->sum_dv * bf->wall )
			{
				bf->new_center_x += bf->x;
				bf->new_center_y += bf->y;

				fig->point_Xs[ fig->point_cnt ] = bf->x;
				fig->point_Ys[ fig->point_cnt ] = bf->y;
				fig->point_cnt++;

				glBegin( GL_LINES );
				glColor3f( 1, 0.4, 0.4 );
				glVertex3f( bf->x - 2, bf->y - 2, 0);
				glVertex3f( bf->x + 2, bf->y + 2, 0);
				glVertex3f( bf->x - 2, bf->y + 2, 0);
				glVertex3f( bf->x + 2, bf->y - 2, 0);
				glEnd();

				return 1; /* есть граница */
			}
		}

		bf->cnt_dv++;
		bf->sum_dv += dv;
	}

	return 0; /* нет границы */
}





static int one_X( img_t * img, maybe_figure * fig, float ang )
{

	border_find_t bf_data;
	bf_data.start_pnt = img->buf + 3 * 
		( ( img->height - fig->enter_y ) * img->width + fig->enter_x );

	bf_data.sum_dv = 0;
	bf_data.cnt_dv = 0;
	bf_data.wall = 22; /* установлено шаманами */
	bf_data.min_dv = 14; /* определено шаманами */
	bf_data.start_x = fig->enter_x;
	bf_data.start_y = fig->enter_y;
	bf_data.new_center_x = 0;
	bf_data.new_center_y = 0;

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

	/* справа */
	if( !mv_border( img, &bf_data, fig, st_x, st_y * (-1) ) )
		return 0; /* Abort */

	/* слева */
	if( !mv_border( img, &bf_data, fig, st_x * (-1), st_y ) )
		return 0;

	/* сверху */
	if( !mv_border( img, &bf_data, fig, st_x * (-1), st_y * (-1) ) )
		return 0;

	/* снизу */
	if( !mv_border( img, &bf_data, fig, st_x, st_y ) )
		return 0;

	fig->enter_x = bf_data.new_center_x / 4;
	fig->enter_y = bf_data.new_center_y / 4;
	return 1; 
}



int X_cycle( img_t * frame, maybe_figure * fig )
{
	int fail_cnt = 0;

	/* Сбросим точки края старой фигурки .. будем искать новую */
	fig->point_cnt = 0;

	int ii;
	for( ii = 0; ii < X_CYCLE_CNT; ii++ )
	{
		if( !one_X( frame, fig, (float)( rand() % 1000 ) * 2 * M_PI / 1000.0 ) )
		{
			if( ( fail_cnt++ ) > ( X_CYCLE_CNT / 50 ) )
			{
				/* Слишком много ошибок подряд */
				return 0; /* A figure is not detected. */
			}
		}
		else
		{
			fail_cnt = 0;
		}
	}

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

