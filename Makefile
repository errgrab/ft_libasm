NAME		:= libasm.a
TEST		:= test_libasm

NASM		:= nasm
NASMFLAGS	:= -f elf64 -g -F dwarf
CC			:= cc
CFLAGS		:= -Wall -Wextra -Werror -g
AR			:= ar rcs
RM			:= rm -rf

SRC_DIR		:= src
BONUS_DIR	:= src_bonus
OBJ_DIR		:= obj
INC_DIR		:= include
TEST_SRC	:= tests/main.c

SRCS		:= ft_strlen.s \
			   ft_strcpy.s \
			   ft_strcmp.s \
			   ft_write.s \
			   ft_read.s \
			   ft_strdup.s \
			   ft_calloc.s

BONUS_SRCS	:= ft_atoi_base_bonus.s \
			   ft_list_push_front_bonus.s \
			   ft_list_size_bonus.s \
			   ft_list_sort_bonus.s \
			   ft_list_remove_if_bonus.s \
			   ft_simd_memchr_bonus.s

OBJS		:= $(addprefix $(OBJ_DIR)/, $(SRCS:.s=.o))
BONUS_OBJS	:= $(addprefix $(OBJ_DIR)/, $(BONUS_SRCS:.s=.o))

all: $(NAME)

$(NAME): $(OBJS)
	$(AR) $@ $^

# "bonus" adds the bonus objects to the same libasm.a.
# The .bonus stamp prevents useless relinking.
bonus: .bonus

.bonus: $(OBJS) $(BONUS_OBJS)
	$(AR) $(NAME) $^
	@touch $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(OBJ_DIR)/%.o: $(BONUS_DIR)/%.s
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

test: $(NAME)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(TEST_SRC) -L. -lasm -o $(TEST)
	./$(TEST)

test_bonus: bonus
	$(CC) $(CFLAGS) -DBONUS -I$(INC_DIR) $(TEST_SRC) -L. -lasm -o $(TEST)
	./$(TEST)

clean:
	$(RM) $(OBJ_DIR) .bonus

fclean: clean
	$(RM) $(NAME) $(TEST)

re: fclean all

.PHONY: all bonus clean fclean re test test_bonus
