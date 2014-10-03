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
#include <time.h>




typedef enum { MP_END, MP_FIL, MP_CLR } mp_type;

typedef struct mark_point
{
	int x;
	int y;
	mp_type type; /* 0-конец 1-точка 2-пусто */
	struct tuzer * last_detected;
} mark_point;


/* +o+ */
/* o+o */
/* o+o */
/* ooo */

/* Первые две точки маркера обязательно непустые */
static mark_point mark1[] = {
{  0, 0, MP_FIL }, {  1, 3, MP_FIL }, { -1, 3, MP_FIL },
{  0, 2, MP_CLR }, { -1, 2, MP_FIL }, {  1, 2, MP_FIL },
{  0, 1, MP_CLR }, { -1, 1, MP_FIL }, {  1, 1, MP_FIL },
{ -1, 0, MP_CLR }, {  1, 0, MP_CLR }, {  0, 3, MP_FIL },
{  0, 0, MP_END }
};





#define USEC_SPOTS    25000/* 25 млсек */
#define USEC_MARKERS  (USEC_SPOTS + 3000)

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
#define X_CYCLE_CNT 200
typedef struct maybe_figure
{
	int enter_x;
	int enter_y;

	float center_x;
	float center_y;

	/* Temporal dataset */
	int point_Xs[ X_CYCLE_CNT * 4 ];
	int point_Ys[ X_CYCLE_CNT * 4 ];
	int point_cnt;

	int near_point_id;
	int far_point_id;

	float angle; /* Угол фигуры */
	float radius; /* Самая дальняя точка фигуры */

	/* Тип фигуры */
	int in_marker;
	int is_circle; /* 1:круг 0:квадрат */
	float press;

} maybe_figure;




/* Для отслеживания кругов и квадратов */

typedef struct tuzer
{
	/* Центр фигуры */
	float x;
	float y;

	/* Вектор скорости */
	float dx;
	float dy;

	/* Точка, где фигура была обнаружена первый раз. */
	float first_x;
	float first_y;

	/* Ожидание первого выхода за радиус */
	int mv_wait; /* -1:движение уже наблюдалось */
	int pause; /* пауза в кадрах до сканирования */

	/* Столько раз подряд ничего нет на x:y */
	int fail_cnt;

	struct tuzer * next;

	struct tuzer * in_cell;
	float radius_square;

	float angle; /* Угол фигуры */
	int is_circle; /* 1:круг 0:квадрат */
	float press;
	float radius; /* Самая дальняя точка фигуры */
	int in_marker;
} tuzer;

/* Около 2 секунды терпим ошибочные координаты тузера */
#define FAIL_TOLERANCE 66

#define TUZER_CNT 12000
static tuzer tuzers[ TUZER_CNT ];
static tuzer * first_tuzer = 0l;

/* Резиновый массив для учета маркерных кругов */
#define CIRCLES_BLOCK 3
static tuzer ** circles = 0l;
static int circles_cnt = 0;
static int max_circles = 0;

static void check_circles_cnt( void )
{
	if( circles_cnt >= max_circles )
	{
		/* Расширить массив */
		tuzer ** n_cir = ( tuzer ** )malloc( sizeof( tuzer * ) * ( max_circles + CIRCLES_BLOCK ) );
		assert( n_cir );
		if( circles )
		{
			memcpy( n_cir, circles, sizeof( tuzer * ) * max_circles );
			free( circles );
		}
		max_circles += CIRCLES_BLOCK;
		circles = n_cir;
	}
}


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

		//c_tuz->x = rand() % width;
		//c_tuz->y = rand() % height;

		c_tuz->fail_cnt = FAIL_TOLERANCE;
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
	if( fig->radius < 4.0 )
		return 0; /* Слишком мелкое пятно */

	fig->angle = asin( fig->point_Ys[ fig->far_point_id ] / fig->radius ); /* от -Pi/2 до +Pi/2 */
	fig->angle = ( fig->point_Xs[ fig->far_point_id ] < 0.0 ) ? ( M_PI - fig->angle ) : fig->angle;

	/* Проверка на окружность */
	float ci_press = 0.5/* Середина шкалы */;
	float step = 0.25/* Половина середины */;
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

	/* Проверка на квадрат */
	float sq_press = 0.5;
	step = 0.25;
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

	if( ( ci_dist > 25.0 ) && ( sq_dist > 25.0 ) ) /* Шаман советует */
		return 0; /* Ни квадрт ни круг */

	fig->is_circle = ( ( ci_dist < sq_dist ) || fig->in_marker ) ? 1/* true */ : 0/* false */;
	fig->press = ( ( ci_dist < sq_dist ) || fig->in_marker ) ? ci_press : sq_press;

	/* Если слишком сплющенный, то отбой */
	if( fig->press < 0.5 )
		return 0;

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

static void show_tuzer( tuzer * c_tuz )
{
	/* Подсчитаем ячейку на карте под это точку и займем площадку */
	int tgt_in_gid = ( ( int )c_tuz->x / GRID_WX ) + tuz_grid_w * ( ( int )c_tuz->y / GRID_WY );
	assert( tgt_in_gid < ( tuz_grid_sz / sizeof( tuzer * ) ) );
	c_tuz->in_cell = tuz_grid[ tgt_in_gid ];
	tuz_grid[ tgt_in_gid ] = c_tuz;

	/* Если фигура-круг, то нам нужно учесть его для маркеров */
	if( c_tuz->is_circle )
	{
		check_circles_cnt();
		circles[ circles_cnt ] = c_tuz;
		circles_cnt++;
	}
		
	/* --------- Рисуем только для отладки ---------- */	
	glBegin( GL_LINES );
	glColor3f( 0.2, 0.2, 1.0 );
	glVertex3f( c_tuz->x, c_tuz->y, 0 );
	glVertex3f( c_tuz->x - c_tuz->dx, c_tuz->y - c_tuz->dy, 0 );

	glColor3f( 1.0, 1.0, 0.3 );
	glVertex3f( c_tuz->x, c_tuz->y, 0 );
	glVertex3f( c_tuz->first_x, c_tuz->first_y, 0 );
	glEnd();

	/* -------- Результаты распознавания нарисуем тут (только для отладки) */
	glColor3f( 0.5, 0.5, 0.5 );
	if( c_tuz->mv_wait > 100 )
		glColor3f( 0.1, 1, 0.1 );
	if( c_tuz->mv_wait == -1 )
		glColor3f( 1, 0.1, 0.1 ); 

	float rad = sqrt( c_tuz->radius_square );

	if( c_tuz->is_circle )
	{
		glBegin( GL_LINE_LOOP );

		float alf;
		for( alf = 0.0; alf < M_PI * 2; alf += M_PI/16 )
		{
			float x = rad * cos( alf );
			float y = rad * sin( alf ) * c_tuz->press;
					
			glVertex3f( x * cos( c_tuz->angle ) - y * sin( c_tuz->angle ) + c_tuz->x, 
				x * sin( c_tuz->angle ) + y * cos( c_tuz->angle ) + c_tuz->y, 0.0 );
		}

		glEnd();
	}
	else
	{
       		glBegin( GL_LINE_LOOP );
		glVertex3f( rad * cos(c_tuz->angle) + c_tuz->x, 
			rad * sin( c_tuz->angle ) + c_tuz->y, 0.0 );

		glVertex3f( c_tuz->press * rad * cos(c_tuz->angle + M_PI/2 ) + c_tuz->x, 
			c_tuz->press * rad * sin( c_tuz->angle + M_PI/2 ) + c_tuz->y, 0.0 );

		glVertex3f( rad * cos(c_tuz->angle + M_PI ) + c_tuz->x, 
			rad * sin( c_tuz->angle + M_PI ) + c_tuz->y, 0.0 );

		glVertex3f( c_tuz->press * rad * cos(c_tuz->angle + M_PI * 1.5 ) 
			+ c_tuz->x, c_tuz->press * rad * sin( c_tuz->angle + M_PI * 1.5 ) 
			+ c_tuz->y, 0.0 );
		glEnd();
	}
}


static tuzer * get_tuzer_4gid( float x, float y, float radius_square, int gid )
{
	if( ( gid < 0 ) || ( ( gid * sizeof( tuzer * ) ) >= tuz_grid_sz ) )
		return 0l;

	tuzer * nr_tuz = tuz_grid[ gid ];
	while( nr_tuz )
	{
		float cq = ( nr_tuz->x - x ) * ( nr_tuz->x - x )
			+ ( nr_tuz->y - y ) * ( nr_tuz->y - y );

		if( cq < radius_square )
			return nr_tuz;

		nr_tuz = nr_tuz->in_cell;
	}
	return 0l;
}

static mp_type check_mark_point( float x, float y, float radius, mark_point * mp )
{
	int iy, ix;
	for( iy = (int)( y - radius ) / GRID_WY; iy <= (int)( y + radius ) / GRID_WY; iy++ )
		for( ix = (int)( x - radius ) / GRID_WX; ix <= (int)( x + radius ) / GRID_WX; ix++ )
			if( ( mp->last_detected = get_tuzer_4gid( x, y, radius * radius, ix + tuz_grid_w * iy ) ) )
				return MP_FIL; /* Есть точка */
	
	return MP_CLR/* Пусто */;
}

static int check_marker( mark_point * mp, int circ1_id, int circ2_id )
{
	if( circ1_id == circ2_id ) return 0;

	/* Координатой маркера будем считать circles[ circ1_id ]->x:y */
	/* Нужно найти, поворот, масштаб */
	/* Расстояние circ1 -> circ2 */
	float r_x = circles[ circ2_id ]->x - circles[ circ1_id ]->x;
	float r_y = circles[ circ2_id ]->y - circles[ circ1_id ]->y;
	float r_l = sqrt( r_x * r_x + r_y * r_y );

	if( r_l < circles[ circ1_id ]->radius ) return 0;

	/* Угол circ1 -> circ2  */
	float r_ang = asin( r_y / r_l );
	r_ang = ( r_x < 0.0 ) ? ( M_PI - r_ang ) : r_ang; 

	/* Расстояние 1 -> 2 шаблона */
	float t_x = mp[ 1 ].x - mp[ 0 ].x;
	float t_y = mp[ 1 ].y - mp[ 0 ].y;
	float t_l = sqrt( t_x * t_x + t_y * t_y );

	/* Угол 1 -> 2 шаблона */
	float t_ang = asin( t_y / t_l );
	t_ang = ( t_x < 0.0 ) ? ( M_PI - t_ang ) : t_ang;
	/* Масштабирование определим */
	float mas = r_l / t_l;

	/* Угол метки определим. ang-PI/2 будет направлением движения бота. */
	float ang = r_ang - t_ang;


	/* Бежим по точкам шаблона пока все совпадает */
	mark_point * c_pnt = mp + 2;
	while( c_pnt->type != MP_END )
	{
		/* Повернуть относительно точки 1 и сместить */
		float tx = ( c_pnt->x - mp[ 0 ].x ) * mas;
		float ty = ( c_pnt->y - mp[ 0 ].y ) * mas;
		
		float x = tx * cos( ang ) - ty * sin( ang );
		float y = tx * sin( ang ) + ty * cos( ang );

		x += circles[ circ1_id ]->x;
		y += circles[ circ1_id ]->y;

		/* x:y - это место на экране, где должна находиться маркерная точка */
		if( check_mark_point( x, y, circles[ circ1_id ]->radius, c_pnt ) != c_pnt->type )
			return 0; /* маркер не совпал */

		/*------------------  Временно в целях отладки рисуем ------------ */
		glBegin( GL_LINES );
		if( c_pnt->type == MP_FIL )
			glColor3f( 0.2, 0.2, 1.0 );
		else
			glColor3f( 0.7, 0.2, 0.2 );
		glVertex3f( circles[ circ1_id ]->x, circles[ circ1_id ]->y, 0 );
		glVertex3f( x, y, 0 );
		glEnd();

		c_pnt++;
	}

	/* Отметим тузеров маркера наградой, ибо они есть маркер */
	mp[ 0 ].last_detected = circles[ circ1_id ];
	mp[ 1 ].last_detected = circles[ circ2_id ];

	c_pnt = mp;
	while( c_pnt->type != MP_END )
	{
		if( c_pnt->last_detected )
			c_pnt->last_detected->in_marker++;

		c_pnt++;
	}

	/* --------------------- Нарисуем для отладки .. ----------------- */
	glBegin( GL_LINES );
	glColor3f( 1.0, 1.0, 1.0 );
	glVertex3f( circles[ circ1_id ]->x, circles[ circ1_id ]->y, 0 );
	glVertex3f( circles[ circ2_id ]->x, circles[ circ2_id ]->y, 0 );
	glEnd();

	glBegin( GL_LINES );
	glColor3f( 1.0, 0.2, 1.0 );
	glVertex3f( circles[ circ1_id ]->x, circles[ circ1_id ]->y, 0 );
	glVertex3f( circles[ circ1_id ]->x + cos( ang - M_PI / 2.0 ) 
		* 60, circles[ circ1_id ]->y + sin( ang - M_PI / 2.0 ) * 60, 0 );
	glEnd();
	printf( "Маркер маркер! ..\n" );


	return 1/* Маркер нашелся */;
}


static int is_time_over( struct timespec * start_tm, long usecs )
{
	struct timespec tm;
	clock_gettime( CLOCK_MONOTONIC, &tm );

	long us = ( tm.tv_sec - start_tm->tv_sec ) * 1000000/* мксек в 1 сек */;
	us += ( tm.tv_nsec - start_tm->tv_nsec ) / 1000/* нсек в 1 мксек */;

	if( us > usecs )
		return 1; /* yes */

	return 0; /* no */
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
	int tm_check = 0;
	struct timespec start_tm;
	clock_gettime( CLOCK_MONOTONIC, &start_tm );

	circles_cnt = 0;
	tuzer * p_tuz = 0l;
	tuzer * c_tuz = first_tuzer;
	while( c_tuz )
	{
		tuz_cnt++;

		if( !( tm_check-- ) )
		{
			tm_check += 25/* только изредка проверяем время */;
			if( is_time_over( &start_tm, USEC_SPOTS ) )
			{
				/* Выходим из цикла поиска пятен */
				printf( "Стоп tuzer-цикл на %i тузер. Кругов: %i\n", tuz_cnt, circles_cnt );
				break;
			}
		}


		if( !c_tuz->fail_cnt && c_tuz->mv_wait > 180/* 6 сек на месте */ )
		{
			if( !c_tuz->pause )
			{
				c_tuz->pause = 33; /* Следующую секунду не проверяем */
			}
			else
			{
				c_tuz->pause--;
				show_tuzer( c_tuz );
				goto jump_up_tuzer;
			}
		}


		c_tuz->fail_cnt++;

		maybe_figure fig;

		fig.enter_x = ( c_tuz->fail_cnt > FAIL_TOLERANCE ) 
			? rand() % img_width : ( c_tuz->x + c_tuz->dx );
		fig.enter_y = ( c_tuz->fail_cnt > FAIL_TOLERANCE ) 
			? rand() % img_height : ( c_tuz->y + c_tuz->dy );

		fig.in_marker = c_tuz->in_marker;
		c_tuz->in_marker = 0/* false */;
		
		/* Свободна ли точка? */
		if( !check_4free( fig.enter_x, fig.enter_y ) )
			goto next_tuzer; /* Значит за пределами экрана */

		if( X_cycle( &frame, &fig ) )
		{
			/* Перепроверим с новым центром */
			if( !check_4free( fig.center_x, fig.center_y ) )
				goto next_tuzer; /* Значит за пределами экрана */

			/* Фигура есть. Сначала определим перемещение. */
			c_tuz->dx = ( c_tuz->fail_cnt > FAIL_TOLERANCE ) ? 0 : fig.center_x - c_tuz->x;
			c_tuz->dy = ( c_tuz->fail_cnt > FAIL_TOLERANCE ) ? 0 : fig.center_y - c_tuz->y;

			c_tuz->x = fig.center_x;
			c_tuz->y = fig.center_y;
			c_tuz->angle = fig.angle;
			c_tuz->is_circle = fig.is_circle;
			c_tuz->press = fig.press;
			c_tuz->radius = fig.radius;

			c_tuz->first_x = ( c_tuz->fail_cnt > FAIL_TOLERANCE ) ? c_tuz->x : c_tuz->first_x;
			c_tuz->first_y = ( c_tuz->fail_cnt > FAIL_TOLERANCE ) ? c_tuz->y : c_tuz->first_y;

			c_tuz->mv_wait = ( c_tuz->fail_cnt > FAIL_TOLERANCE ) ? 0 : c_tuz->mv_wait;

			c_tuz->radius_square = fig.radius * fig.radius;

			/* Детектор движения пятнышка  */
			float jmp_square = ( c_tuz->first_x - c_tuz->x ) * ( c_tuz->first_x - c_tuz->x )
					+ ( c_tuz->first_y - c_tuz->y ) * ( c_tuz->first_y - c_tuz->y );

			c_tuz->mv_wait = ( jmp_square > c_tuz->radius_square ) ? -1 : c_tuz->mv_wait;
			c_tuz->mv_wait = ( c_tuz->mv_wait != -1 ) ? c_tuz->mv_wait + 1 : c_tuz->mv_wait;

			c_tuz->fail_cnt = 0;

			show_tuzer( c_tuz );
			goto jump_up_tuzer;
		}

next_tuzer:
		p_tuz = c_tuz;
		c_tuz = c_tuz->next;
		continue;

jump_up_tuzer:
		/* Поднимем тузера. */
		if( p_tuz )
		{
			p_tuz->next = c_tuz->next;
			c_tuz->next = first_tuzer;
			first_tuzer = c_tuz;
	
			c_tuz = p_tuz->next;
			continue;
		}
		/* Это случается если тузер и так уже на верху*/
		goto next_tuzer;
	}

	/* Ищем маркеры если есть круги в поле зрения */
	if( circles_cnt >= 2/* Минимум 2 круга */ )
	{
		int mrk_fcnt = 0;
		while( 1 )
		{ 
			mrk_fcnt++;
			if( !( tm_check-- ) )
			{
				tm_check += 2500/* только изредка проверяем время */;
				if( is_time_over( &start_tm, USEC_MARKERS ) )
				{
					/* Выходим из цикла маркеров */
					printf( "Стоп маркер-цикл на %i пробе.\n", mrk_fcnt );
					//printf( "circle cnt=%i .. max=%i\n", circles_cnt, max_circles );
					break;
				}
			}

			/* Выбираем случайно 2 круга и накладываем шаблон */
			if( check_marker( mark1, rand() % circles_cnt, rand() % circles_cnt ) )
			{
				/* Нашелся маркер.. */
				break;
			}
		}
	}



	/* ----------- Распечатаем грид (только для отладки) ----------- */
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

