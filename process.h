#ifndef PROCESS_H_
#define PROCESS_H_


/* Один раз перед началом работы необходимо 
 * проинициализировать все структуры данных 
 *    ( width, height ) - размеры фреймов */
void init_img_processor( int width, int height );


/* Сделать все наши вычисления над кадром img */
void process_rgb_frame( uint8_t *img );



#endif /*PROCESS_H_*/
