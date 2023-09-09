/**
 * @file   messenger.hpp
 * @author 
 * @brief  Задание1 - реализация протокола обмена сообщениями между пользователями.
 *
 * @detail Для обмена сообщениями между пользователями используются пакеты следующего вида:
 * 
 *	+-MSG packet------------------------------------------------------------------------------------------>
 *	0        2 3          6 7           11 12     15 16        
 *	+---------+------------+--------------+---------+----------------------+------------------------------+
 *	|  FLAG   | NAME_LEN   |    MSG_LEN   |  CRC4   |        NAME          |            MSG               |
 *	+---------+------------+--------------+---------+----------------------+------------------------------+
 *
 *	FLAG		- [3 bits]			- флаг содержащий двоичное значение 101;
 *	NAME_LEN	- [4 bits]			- содержит количество символов в поле NAME (без символа окончания строки), возможные значения: [1:15];
 *  MSG_LEN		- [5 bits]			- содержит количество символов в поле MSG (без символа окончания строки), возможные значения: [1:31];
 *  CRC4		- [4 bits]			- содержит значение CRC4 для полей: FLAG, NAME_LEN, MSG_LEN, NAME, MSG;
 *	NAME		- [NAME_LEN bytes]	- содержит имя отправителя (без символа окончания строки);
 *	MSG  		- [MSG_LEN bytes]	- содержит текст сообщения (без символа окончания строки).
 *
 * При этом:
 *		1) в случае если текст отправляемого сообщения не может поместиться в 1 пакет, сообщение необходимо упаковать несколькими пакетами;
 *		2) в случае если имя отправителя сообщения превышает максимальный допустимый размер бросить исключение std::length_error;
 *		3) имя отправителя и текст сообщения не могут быть пустыми - в случае нарушения условия бросить исключение std::length_error;
 *		4) значение CRC4 рассчитывается используя код http://read.pudn.com/downloads169/sourcecode/math/779571/CRC4.C__.htm
 */
#ifndef TASK1_MESSENGER_HPP
#define TASK1_MESSENGER_HPP

#include <stdint.h>
#include <stdexcept>
#include <iterator>		// std::advance, std::distance
#include <cassert>
#include <vector>
#include <string>

namespace messenger
{

/**
 * Helper type to represent message: sender name, message text
 */
struct msg_t
{
	// Does it copy-initialize name & text (?) I think no...
	msg_t(const std::string & nm, const std::string & txt)
		: name(nm)
		, text(txt)
	{}

	// Move-like initialization (?)
	// msg_t(std::string &&nm, std::string &&txt)
	// 	: name(nm)
	// 	, text(txt)
	// {}

	std::string name;	/**< message sender's name */
	std::string text;	/**< message text */
};


/**
 * Prepare raw message buffer from specified message
 *
 * @note raw message buffer may consist from several (at least one) message packets
 *
 * @param msg message sender's name & message text
 * @return buffer with prepared message packets
 *
 * @sample
 * 
 * // if make_buff succeeded: buff contains required number of packets (for this case 1) to encode specified message
 * std::vector<uint8_t> buff = messenger::make_buff( messenger::msg_t("Timur", "Hi") );
 *
 * // if parse_buff succeeded:
 * //	msg.name should be "Timur";
 * //	msg.text should be "Hi".
 * messenger::msg_t msg = messenger::parse_buff(buff);
*/
std::vector<uint8_t> make_buff(const msg_t & msg);


/**
* Parse specified raw message buffer to get original message
*
* @param buff raw message buffer
* @return parsed message
*
* @note In the process of decoding the buffer, it is necessary to verify the value of the fields:
*	- FLAG;
*	- CRC4.
* If their value will be incorrect throw std::runtime_error
*/
msg_t parse_buff(std::vector<uint8_t> & buff);


}	// namespace messenger

#endif // !TASK1_MESSENGER_HPP
