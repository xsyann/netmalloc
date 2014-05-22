##
## Makefile
##
## Made by xsyann
## Contact <contact@xsyann.com>
##
## Started on  Fri May  9 11:46:05 2014 xsyann
## Last update Wed May 21 17:41:18 2014 xsyann
##

TARGET	= test
SRC	= test.c
OBJ	= $(SRC:.c=.o)
CC	= gcc
LDFLAGS += -lpthread
CFLAGS	+= -W -Wall
RM	= rm -vf

$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

all	: $(TARGET)

clean	:
	$(RM) $(OBJ) *~ \#*#

fclean	: clean
	$(RM) $(TARGET)

re	: fclean all clean

.PHONY	: clean fclean re
