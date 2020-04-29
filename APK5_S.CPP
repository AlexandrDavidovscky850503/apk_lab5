#include <locale.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <Windows.h>

// Макро для выдачи звукового сигнала
#define PLAY_SOUND() _asm { \
_asm mov bx, 0      \
_asm mov ax, 0E07h  \
_asm int 10h     \
}

unsigned int globalCount = 0;

void interrupt(*oldVector)(...);
void interrupt newVector(...);
void interrupt(*oldAlarm) (...);

void getTime();
void getDate();

void reset();
void setTime(int flag);
void mySleep(unsigned int milliseconds);

int bcdToDecimal(int number);
int decimalToBCD(int number);

int main(){
      //	SetConsoleCP(1251);
      // setlocale(LC_ALL, "Russian");
      //	SetConsoleOutputCP(1251);
	while (1){
		getDate();
		getTime();
		printf("1->set Time\n");
		printf("2->set Alarm\n");
		printf("3->delay\n");
		printf("0->Exit\n\n");

		switch (getch()){
			case '1': setTime(1); break;
			case '2': setTime(0); break;
			case '3': int milliseconds;
					  rewind(stdin);
					  printf("enter ms: ");
					  scanf("%d", &milliseconds);
					  printf("DELAY...\n");
					  mySleep(milliseconds);
					  printf("continuation of work\n");
					  break;
			case '0': reset(); return 0;
			default: continue;
		}
	}
}

void reset() {
	if (oldAlarm != NULL) {
		setvect(0x4A, oldAlarm);
		outp(0xA1, (inp(0xA0) | 0x01)); // запретить прерывание часов реального времени

		outp(0x70, 0x05);	//часы
		outp(0x71, 0x00);	//сброс

		outp(0x70, 0x03);	//минуты
		outp(0x71, 0x00);

		outp(0x70, 0x01);	//секунды
		outp(0x71, 0x00);

		outp(0x70, 0xB);
		outp(0x71, (inp(0x71) & 0xDF)); // блокировать прерывание будильника
	}
}

void _interrupt newVector(...) {
	globalCount++;
	oldVector();
	//outp(0x70, 0x0C);
	//inp(0x71);

	//outp(0x20, 0x20);
	//outp(0xA0, 0x20);
}

void interrupt newAlarm(...) {
	write(1, "\n!!! WORKS !!!", 17);
       //	PLAY_SOUND();
       //	PLAY_SOUND();
       //	PLAY_SOUND();
       //	PLAY_SOUND();
       //	PLAY_SOUND();
       //	PLAY_SOUND();
       //	PLAY_SOUND();
      sound(500);
      delay(50);
      nosound();
}

int bcdToDecimal(int number){
	return ((number / 16 * 10) + (number % 16));
}

int decimalToBCD(int number) {
	return ((number / 10 * 16) + (number % 10));
}

void getTime() {
	outp(0x70, 0x04); //часы
	printf("%03d:", bcdToDecimal(inp(0x71)));
	outp(0x70, 0x02); //минуты
	printf("%03d:", bcdToDecimal(inp(0x71)));
	outp(0x70, 0x00); //секунды
	printf("%03d\n", bcdToDecimal(inp(0x71)));
}

void getDate() {
	outp(0x70, 0x07); //день
	printf("%02d.", bcdToDecimal(inp(0x71)));
	outp(0x70, 0x08); //месяц
	printf("%02d.", bcdToDecimal(inp(0x71)));
	//outp(0x70, 0x32); //century
	//printf("%02d", bcdToDecimal(inp(0x71)));
	outp(0x70, 0x09); //год
	printf("%04d\n", bcdToDecimal(inp(0x71)));
}

void setTime(int flag) {
	int hours;
	do {
		rewind(stdin);
		printf("Hours: ");
		scanf("%i", &hours);
	} while ((hours > 23 || hours < 0));
	hours = decimalToBCD(hours);

	int minutes;
	do {
		rewind(stdin);
		printf("Minutes: ");
		scanf("%i", &minutes);
	} while (minutes > 59 || minutes < 0);
	minutes = decimalToBCD(minutes);

	int seconds;
	do {
		rewind(stdin);
		printf("Seconds: ");
		scanf("%i", &seconds);
	} while (seconds > 59 || seconds < 0);
	seconds = decimalToBCD(seconds);

	if (flag) {//режим часы
		int i = 0;
		outp(0x70, 0x0A);
		for (; (inp(0x71) & 0x80) && i < 10000; i++)  //седьмой бит сообщает, что данные времени готовы к извлечению, если 0
			outp(0x70, 0x0A);

		if (i == 10000) {
			printf("ERROR");
			return;
		}

		outp(0x70, 0xB);
		outp(0x71, inp(0x71) | 0x80); // отрицать обновление времени

		outp(0x70, 0x04);
		outp(0x71, hours);
		outp(0x70, 0x02);
		outp(0x71, minutes);
		outp(0x70, 0x00);
		outp(0x71, seconds);

		outp(0x70, 0xB);
		outp(0x71, inp(0x71) & 0x7F); // разрешить обновление RTC
	}
	else {//режим будильник
		outp(0x70, 0x05);
		outp(0x71, hours);
		outp(0x70, 0x03);
		outp(0x71, minutes);
		outp(0x70, 0x01);
		outp(0x71, seconds);

		outp(0x70, 0xB);
		outp(0x71, (inp(0x71) | 0x20)); // разрешить прерывание будильника

		oldAlarm = getvect(0x4A);
		setvect(0x4A, newAlarm);
		outp(0xA1, (inp(0xA0) & 0xFE));	// разрешить прерывание RTC
	}
}

void mySleep(unsigned int milliseconds) {
	disable();

	oldVector = getvect(0x70);
	setvect(0x70, newVector);

	enable();

	outp(0xA1, inp(0xA1) & 0xFE);

	outp(0x70, 0xB);
	outp(0x71, inp(0x71) | 0x40);

	globalCount = 0;

	while (globalCount <= milliseconds);

	disable();

	setvect(0x70, oldVector);

	enable();

	return;
}