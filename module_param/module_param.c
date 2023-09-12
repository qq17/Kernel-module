#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <linux/limits.h>
#include <errno.h>
#include <termios.h>

#define PATH_VAR_FILE "/sys/kernel/kernel_module/path"
#define PERIOD_VAR_FILE "/sys/kernel/kernel_module/period"

// ЗАПУСКАТЬ НЕОБХОДИМО С ПОМОЩЬЮ sudo

// читает файл в /sys/kernel/kernel_module/, связанный с переменной модуля ядра
int read_var_file(char * var_file)
{
	FILE * fd = fopen(var_file, "r");
	char buffer[PATH_MAX];
	if (fd == NULL)
	{
		printf("Can't open file %s\nAre you root?\n", var_file);
		return -1;
	}
	while(fgets(buffer, PATH_MAX, fd))
	{
		printf("%s\n", buffer);
	}
	fclose(fd);
	return 0;
}

// задает для модуля ядра новый период в секундах, записывая его в файл /sys/kernel/kernel_module/period
int write_period(char * period_sec_var_file, unsigned int period)
{
	FILE * fd_period = fopen(period_sec_var_file, "w");
	if (fd_period == NULL)
	{
		printf("Can't open file %s\nAre you root?\n", period_sec_var_file);
		return -1;
	}
	fprintf(fd_period, "%u", period);
	fclose(fd_period);
	printf("Period %u was written in %s\n", period, period_sec_var_file);
	return 0;
}

// задает для модуля ядра новое название файла, записывая его в файл /sys/kernel/kernel_module/path
int write_path(char * path_var_file, char * path)
{
	FILE * fd_path = fopen(path_var_file, "w");
	if (fd_path == NULL)
	{
		printf("Can't open file %s\nAre you root?\n", path_var_file);
		return -1;
	}
	fprintf(fd_path, "%s", path);
	fclose(fd_path);
	printf("Path %s was written in %s\n", path, path_var_file);
	return 0;
}

// обрабатывает ввод периода в секундах (unsigned int)
unsigned int scan_period(void)
{
	// в модуле ядра для установки времени срабатывания таймера в функции msecs_to_jiffies()
	// используются миллисекунды, аргумент типа unsigned int, поэтому максимально допустимое значение
	// периода в секундах = UINT_MAX / 1000 = 4 294 967 295 / 1000 = 4 294 967
	char buffer[9];
	char *buffer_end;
	unsigned long num_ul;
	unsigned int period;
	int c;

	while (true)
	{
		printf("Enter period in seconds\n");
		if (fgets(buffer, sizeof buffer, stdin) == NULL)
		{
			fprintf(stderr, "Unrecoverable error reading from input!\n");
			exit(EXIT_FAILURE);
		}

		// проверка того, что ввод не слишком длинный
		if (strchr(buffer, '\n') == NULL)
		{
			printf("Input was too long!\n");
			while ((c = getchar()) != '\n' && c != EOF) { } // очистка stdin
			buffer[0] = '\0';
			continue;
		}

		// попытка перевести введенную строку в число
		errno = 0;
		num_ul = strtoul(buffer, &buffer_end, 10);
		if (buffer_end == buffer)
		{
			printf("Error converting string to number\n");
			buffer[0] = '\0';
			continue;
		}

		// проверка диапазона значений
		if (errno == ERANGE || num_ul > (unsigned int)(UINT_MAX/1000) || num_ul == 0)
		{
			printf("Range error! Max period is %u seconds and can't be 0\n", (unsigned int)(UINT_MAX/1000));
			buffer[0] = '\0';
			continue;
		}

		// проверка того, что после числа не были введены значимые символы,
		// то ечть отбрасывается ввод типа "123 12", "123asd"
		bool UNEXP_INPUT = false;
		for ( ; *buffer_end != '\0'; buffer_end++)
		{
			if (!isspace((unsigned char)*buffer_end))
			{
				printf("Unexpected input encountered!\n");
				buffer[0] = '\0';
				UNEXP_INPUT = true;
				break;
			}
		}
		if (UNEXP_INPUT)
		{
			continue;
		}

		// все проверки пройдены, введенный период удовлетворяет условиям
		period = (unsigned int)num_ul;
		return period;
	}
}

// обрабатывает ввод пути к файлу, в который нужно дописывать строки
int scan_path(char * path, unsigned int maxsize)
{
	int c;

	if (path == NULL || maxsize < 1)
	{
		return -1;
	}
	
	while (true)
	{
		printf("Enter path to file to append lines \"Hello from kernel module\"\n");
		if (fgets(path, maxsize, stdin) == NULL)
		{
			fprintf(stderr, "Unrecoverable error reading from input!\n");
			exit(EXIT_FAILURE);
		}

		// проверка того, что ввод не слишком длинный
		if (strchr(path, '\n') == NULL)
		{
			printf("Input was too long for path!\n");
			while ((c = getchar()) != '\n' && c != EOF) { }
			path[0] = '\0';
			continue;
		}

		return 0;
	}
}

// getchar без необходимости ждать нажатия ENTER
int getch(void)
{
	int ch;
	struct termios old, current;
	tcgetattr(0, &old); // запоминаем старые настройки
	current = old;
	current.c_lflag &= ~ICANON; // отключаем строчную буферизацию
	tcsetattr(0, TCSANOW, &current); // меняем настройки

	ch = getchar();

	tcsetattr(0, TCSANOW, &old); // восстанавливаем настройки
	return ch;
}

int main( void )
{
	unsigned int period;
	char path[PATH_MAX];

	printf("Press 0 to exit\n");
	printf("Press 1 to read current period value in seconds\n");
	printf("Press 2 to read current path to file to append lines \"Hello from kernel module\"\n");
	printf("Press 3 to pass new period value to the kernel module\n");
	printf("Press 4 to pass new path value to the kernel module\n");

	while(true)
	{
		switch(getch())
		{
			case '0':
				// завершение работы программы
				return 0;
			case '1':
				// чтение текущего установленного периода таймера модуля ядра
				printf("Current period: ");
				read_var_file(PERIOD_VAR_FILE);
				break;
			case '2':
				// чтение текущего установленного пути к файлу, куда дописываются строки
				printf("Current path: ");
				read_var_file(PATH_VAR_FILE);
				break;
			case '3':
				// ввод и установка нового периода
				period = scan_period();
				printf("You entered the following period: %u\n", period);
				write_period(PERIOD_VAR_FILE, period);
				break;
			case '4':
				// ввод и установка нового пути к файлу
				scan_path(path, PATH_MAX);
				printf("You entered the following path: %s\n", path);
				write_path(PATH_VAR_FILE, path);
				break;
			default:
				continue;
		}
	}
}
