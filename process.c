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

	int near_point_id;
	int far_point_id;

	float angle; /* Угол фигуры */
	float radius; /* Самая дальняя точка фигуры */

	/* Тип фигуры */
	int is_circle; /* 1:круг 0:квадрат */
	float press;

} maybe_figure;




/* Для отслеживания кругов и квадратов */

typedef struct tuzer
{
	int x;
	int y;

	int dx;
	int dy;

	struct tuzer * next;

	struct tuzer * in_cell;
	float radius_square;
} tuzer;

#define TUZER_CNT 1200
#define STABLE_TUZERS 800
static tuzer tuzers[ TUZER_CNT ];
static tuzer * first_tuzer = 0l;


/* Подготовиться к работе */
static int img_height;
static int img_width;


#define GRID_WX 25
#define GRID_WY 25
static tuzer ** tuz_grid = 0l;
static int tuz_grid_w = 0;
static int tuz_grid_sz = 0;

void init_img_processor( int width, int height )
{
	img_height = height;
	img_width = width;

	/* Готовим тузеров */
	int i;
	for( i = 0; i < TUZER_CNT; i++ )
	{
		tuzer * c_tuz = tuzers + i;
		memset( c_tuz, 0, sizeof( tuzer ) );
		
		c_tuz->next = first_tuzer;
		first_tuzer = c_tuz;

		c_tuz->x = rand() % width;
		c_tuz->y = rand() % height;
	}
	
	/* Сеть для поиска близких тузеров */
	tuz_grid_w = width / GRID_WX + 1;
	tuz_grid_sz = tuz_grid_w * ( height / GRID_WY + 1 ) * sizeof( tuzer* );
	printf( "debug: Размер tuz_grid: %i\n", tuz_grid_sz );

	tuz_grid = ( tuzer ** )malloc( tuz_grid_sz );
	memset( tuz_grid, 0, tuz_grid_sz );
}




static int mv_border( img_t * img, border_find_t * bf, maybe_figure * fig, float st_x, float st_y )
{
	int i;

	for( i = 1; i < RAY_LEN; i++ )
	{
		bf->x = bf->start_x + ( int )( i * st_x ); 
		bf->y = bf->start_y - ( int )( i * st_y ); 


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

				//glBegin( GL_LINES );
				//glColor3f( 1, 0.4, 0.4 );
				//glVertex3f( bf->x - 2, bf->y - 2, 0);
				//glVertex3f( bf->x + 2, bf->y + 2, 0);
				//glVertex3f( bf->x - 2, bf->y + 2, 0);
				//glVertex3f( bf->x + 2, bf->y - 2, 0);
				//glEnd();

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
	bf_data.wall = 18; /* 22 установлено шаманами */
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


static float maybe_sphere( maybe_figure * fig, float press )
{
	float ret_s = 0.0;
	float angle = fig->angle * (-1.0);

	int i;
	for( i = 0; i < fig->point_cnt; i++ )
	{
		/* Для начала повернем на angle радианов */
		float sx = fig->point_Xs[ i ] * cos( angle )
			- fig->point_Ys[ i ] * sin( angle );
		float sy = fig->point_Xs[ i ] * sin( angle )
			+ fig->point_Ys[ i ] * cos( angle );

		/* Затем разожмем */
		sy /= press;

		/* Теперь найдем длину получившегося луча */
		float l = sqrt( sx * sx + sy * sy );
		ret_s += ( fig->radius - l ) * ( fig->radius - l );
	}
	ret_s /= fig->point_cnt;
	return ret_s;
}

static float maybe_square( maybe_figure * fig, float press )
{
	float ret_s = 0.0;
	float angle = fig->angle * (-1.0);

	int i;
	for( i = 0; i < fig->point_cnt; i++ )
	{
		/* Для начала повернем на angle радианов */
		float sx = fig->point_Xs[ i ] * cos( angle )
			- fig->point_Ys[ i ] * sin( angle );
		float sy = fig->point_Xs[ i ] * sin( angle )
			+ fig->point_Ys[ i ] * cos( angle );

		/* Затем разожмем */
		sy /= press;

		/* Теперь найдем найдем нужную точку на идеальной фигуре */
		float a = 1.0;
		float b = fig->radius;

		if( ( sx > 0 ) && ( sy > 0 ) )
		{
			a *= -1.0;
		}
		else if( ( sx < 0 ) && ( sy < 0 ) )
		{
			a *= -1.0;
			b *= -1.0;
		}
		else if( ( sx > 0 ) && ( sy < 0 ) )
		{
			b *= -1.0;
		}

		float fx = 0.0;
		float fy = b;

		if( abs( sx ) > 0.00001 /* проверка на 0 */ )
		{
			fx = ( b * (-1) / ( a - sy / sx ) );
			fy = a * fx + b;
		}

		ret_s += ( fx - sx ) * ( fx - sx ) + ( fy - sy ) * ( fy - sy );
	}

	ret_s /= fig->point_cnt;
	return ret_s;
}

static int X_cycle( img_t * frame, maybe_figure * fig )
{
	int fail_cnt = 0;

	/* Сбросим точки края старой фигурки .. будем искать новую */
	fig->point_cnt = 0;

	int ii;
	for( ii = 0; ii < X_CYCLE_CNT; ii++ )
	{
		if( !one_X( frame, fig, (float)( rand() % 1000 ) * 2 * M_PI / 1000.0 ) )
		{
			if( ( fail_cnt++ ) > ( X_CYCLE_CNT / 33 ) )
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

	/* Определить что это за фигура */
	float near_point = 0.0;
	float far_point = 0.0;
	float last_s = 0.0;
	for( i = 0; i < fig->point_cnt; i++ )
	{
		fig->point_Xs[ i ] -= fig->center_x;
		fig->point_Ys[ i ] -= fig->center_y;
		
		last_s = fig->point_Xs[ i ] * fig->point_Xs[ i ]
			+ fig->point_Ys[ i ] * fig->point_Ys[ i ];

		if( ( last_s < near_point ) || ( !i ) )
		{
			near_point = last_s;
			fig->near_point_id = i;
		}

		if( far_point < last_s )
		{
			far_point = last_s;
			fig->far_point_id = i;
		}
	}

	/* Носом фигуры считаем fig->far_point_id ... Далее определяем тип фигурки */
	
	/* Определяем угол для носа */

	fig->radius = fig->point_Xs[ fig->far_point_id ] * fig->point_Xs[ fig->far_point_id ]; 
	fig->radius += fig->point_Ys[ fig->far_point_id ] * fig->point_Ys[ fig->far_point_id ];
	fig->radius = sqrt( fig->radius );
	if( fig->radius < 1.0 )
		return 0; /* Слишком мелкое пятно */

	fig->angle = asin( fig->point_Ys[ fig->far_point_id ] / fig->radius ); /* от -Pi/2 до +Pi/2 */
	fig->angle = ( fig->point_Xs[ fig->far_point_id ] < 0.0 ) ? ( M_PI - fig->angle ) : fig->angle;


	/* Проверка на квадрат */
	float sq_press = 0.5;
	float step = 0.25;
	float sq_dist = maybe_square( fig, sq_press );

	for( i = 0; i < 6/* Число итераций */; i++ )
	{
		float left_dist = maybe_square( fig, sq_press - step );
		float right_dist = maybe_square( fig, sq_press + step );

		if( left_dist < sq_dist )
		{
			sq_dist = left_dist;
			sq_press -= step;
		}

		if( right_dist < sq_dist )
		{
			sq_dist = right_dist;
			sq_press += step;
		}

		step /= 2.0;
	}

	/* Проверка на окружность */
	float ci_press = 0.5;
	step = 0.25;
	float ci_dist = maybe_sphere( fig, ci_press );

	for( i = 0; i < 6/* Число итераций */; i++ )
	{
		float left_dist = maybe_sphere( fig, ci_press - step );
		float right_dist = maybe_sphere( fig, ci_press + step );

		if( left_dist < ci_dist )
		{
			ci_dist = left_dist;
			ci_press -= step;
		}

		if( right_dist < ci_dist )
		{
			ci_dist = right_dist;
			ci_press += step;
		}

		step /= 2.0;
	}

	fig->is_circle = ( ci_dist < sq_dist ) ? 1/* true */ : 0/* false */;
	fig->press = ( ci_dist < sq_dist ) ? ci_press : sq_press;

	return 1;
}


static int check_4free( int x, int y )
{
	int gid = ( x / GRID_WX ) + tuz_grid_w * ( y / GRID_WY );
	if( ( gid < 0 ) || ( ( gid * sizeof( tuzer * ) ) >= tuz_grid_sz ) )
		return 0; /* Плохая точка за пределами экрана */

	tuzer * nr_tuz = tuz_grid[ gid ];
	while( nr_tuz )
	{
		float cq = ( nr_tuz->x - x ) * ( nr_tuz->x - x )
			+ ( nr_tuz->y - y ) * ( nr_tuz->y - y );

		if( cq < nr_tuz->radius_square )
			return 0; /* Занято */

		nr_tuz = nr_tuz->in_cell;
	}
	
	return 1; /* Свободно */
}

void process_rgb_frame( uint8_t *img )
{

	img_t frame;
	frame.buf = img;
	frame.width = img_width;
	frame.height = img_height;

	/* Очистим сеть поиска тузеров */
	memset( tuz_grid, 0, tuz_grid_sz );


	/* Пробегаемся по всем вероятным точкам в поисках кругов и квадратов */
	int tuz_cnt = 0;
	tuzer * p_tuz = 0l;
	tuzer * c_tuz = first_tuzer;
	while( c_tuz )
	{
		tuz_cnt++;

		maybe_figure fig;
		int new_random_enter = ( tuz_cnt < STABLE_TUZERS ) ? 0 : 1;

		fig.enter_x = new_random_enter ? rand() % img_width : ( c_tuz->x + c_tuz->dx );
		fig.enter_y = new_random_enter ? rand() % img_height : ( c_tuz->y + c_tuz->dy );
		fig.type = 0/* unknown figure */;
		
		/* Свободна ли точка? */
		if( !check_4free( fig.enter_x, fig.enter_y ) )
			goto next_tuzer; /* Значит за пределами экрана */

		if( X_cycle( &frame, &fig ) )
		{
			float near_square = fig.point_Xs[ fig.near_point_id ]
				* fig.point_Xs[ fig.near_point_id ]
				+ fig.point_Ys[ fig.near_point_id ]
				* fig.point_Ys[ fig.near_point_id ];

			float far_square = fig.point_Xs[ fig.far_point_id ]
				* fig.point_Xs[ fig.far_point_id ]
				+ fig.point_Ys[ fig.far_point_id ]
				* fig.point_Ys[ fig.far_point_id ];


			if( far_square < 4.0 )
				goto next_tuzer; /* Подозрительно мелкий */

			if( near_square * 49.0 < far_square )
				goto next_tuzer; /* Подозрительно сплющенный */

			/* Перепроверим с новым центром */
			if( !check_4free( fig.center_x, fig.center_y ) )
				goto next_tuzer; /* Значит за пределами экрана */

			/* Фигура есть. Координаты запомним. */
			c_tuz->dx = new_random_enter ? 0 : fig.center_x - c_tuz->x;
			c_tuz->dy =  new_random_enter ? 0 : fig.center_y - c_tuz->y;
			c_tuz->x = fig.center_x;
			c_tuz->y = fig.center_y;

			c_tuz->radius_square = far_square;

			//if( c_tuz->radius_square < 4.0 )
			//	goto next_tuzer; /* Совсем шум */

			/* Подсчитаем ячейку на карте под это точку */
			int tgt_in_gid = ( c_tuz->x / GRID_WX ) + tuz_grid_w * ( c_tuz->y / GRID_WY );
			assert( tgt_in_gid < ( tuz_grid_sz / sizeof( tuzer * ) ) );
			c_tuz->in_cell = tuz_grid[ tgt_in_gid ];
			tuz_grid[ tgt_in_gid ] = c_tuz;

		
			/* --------- Рисуем только для отладки ---------- */	
			glBegin( GL_LINES );
			glColor3f( 0.2, 0.2, 1.0 );
			glVertex3f( fig.center_x, fig.center_y, 0 );
			glVertex3f( fig.center_x - c_tuz->dx, fig.center_y - c_tuz->dy, 0 );

			glColor3f( 1.0, 1.0, 0.3 );
			glVertex3f( fig.center_x, fig.center_y, 0 );
			glVertex3f( fig.center_x + fig.point_Xs[ fig.far_point_id ], 
				fig.center_y + fig.point_Ys[ fig.far_point_id ], 0 );
			glEnd();

			
			/* -------- Результаты распознавания нарисуем тут (только для отладки) */
		        if( tuz_cnt < STABLE_TUZERS )
				glColor3f( 0.7, 0, 0 );
			else
				glColor3f( 1, 0.1, 0.1 );

			if( fig.is_circle )
			{
				glBegin( GL_LINE_LOOP );
				glColor3f( 1.0, 0.0, 0.0 );

				float alf;
				for( alf = 0.0; alf < M_PI * 2; alf += M_PI/16 )
				{
					float x = fig.radius * cos( alf );
					float y = fig.radius * sin( alf ) * fig.press;
					
					glVertex3f( x * cos( fig.angle ) - y * sin( fig.angle ) + fig.center_x, 
						x * sin( fig.angle ) + y * cos( fig.angle ) + fig.center_y, 0.0 );
				}

				glEnd();
			}
			else
			{
        			glBegin( GL_LINE_LOOP );
				glColor3f( 1.0, 0.0, 0.0 );
				glVertex3f( fig.radius * cos(fig.angle) + fig.center_x, 
					fig.radius * sin( fig.angle ) + fig.center_y, 0.0 );

				glVertex3f( fig.press * fig.radius * cos(fig.angle + M_PI/2 ) + fig.center_x, 
					fig.press * fig.radius * sin( fig.angle + M_PI/2 ) + fig.center_y, 0.0 );

				glVertex3f( fig.radius * cos(fig.angle + M_PI ) + fig.center_x, 
					fig.radius * sin( fig.angle + M_PI ) + fig.center_y, 0.0 );

				glVertex3f( fig.press * fig.radius * cos(fig.angle + M_PI * 1.5 ) 
					+ fig.center_x, fig.press * fig.radius * sin( fig.angle + M_PI * 1.5 ) 
					+ fig.center_y, 0.0 );
				glEnd();
			}

			
			/* Поднимем тузера. */
			if( p_tuz )
			{
				p_tuz->next = c_tuz->next;
				c_tuz->next = first_tuzer;
				first_tuzer = c_tuz;

				c_tuz = p_tuz->next;
				continue;
			}
		}
next_tuzer:
		p_tuz = c_tuz;
		c_tuz = c_tuz->next;
	}

	/* ----------- Распечатаем грид (только для отладки) */
	int ix,iy;
	for( iy = 0; iy < img_height / GRID_WY; iy++ )
	{
		for( ix = 0; ix < img_width / GRID_WX; ix++ )
		{
			glBegin( GL_LINES );
			glColor3f( 0.2, 1.0, 1.0 );

			assert( iy * tuz_grid_w + ix < ( tuz_grid_sz / sizeof( tuzer * ) ) );

			c_tuz = tuz_grid[ iy * tuz_grid_w + ix ];
			while( c_tuz )
			{
				glVertex3f( c_tuz->x, c_tuz->y, 0 );
				glVertex3f( ix * GRID_WX, iy * GRID_WY, 0 );

				c_tuz = c_tuz->in_cell;
			}
			glEnd();
		}
	}
}

