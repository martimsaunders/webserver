# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: praders <praders@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/11/12 12:09:56 by praders           #+#    #+#              #
#    Updated: 2026/02/17 16:53:14 by praders          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv

SRCS = src/src_conf_pars/ParseConfig.cpp src/src_conf_pars/Parser.cpp \
src/src_conf_pars/testConfig.cpp src/src_conf_pars/Tokenizer.cpp \
src/src_conf_pars/ValidateConfig.cpp \
src/main.cpp \
src/src_server/webserv.cpp \

OBJ_DIR = obj
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))

CPP = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++98 -g -Iinc

GREEN = \033[0;32m
BLUE = \033[0;34m
RED = \033[0;31m
YELLOW = \033[0;33m
MAGENTA = \033[0;35m
CYAN = \033[0;36m
PURPLE = \033[0;35m
WHITE = \033[0;37m
RESET = \033[0m
BOLD_GREEN = \033[1;32m
BOLD_BLUE = \033[1;34m
BOLD_RED = \033[1;31m
BOLD_YELLOW = \033[1;33m
BOLD_MAGENTA = \033[1;35m
BOLD_CYAN = \033[1;36m
BOLD_WHITE = \033[1;97m

all: $(NAME)

$(OBJ_DIR):
	@mkdir -p $@

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "$(BOLD_BLUE)Compilando $<...$(RESET)"
	@$(CPP) $(CPPFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	@echo "$(BOLD_CYAN)Linkando $(NAME)...$(RESET)"
	@$(CPP) $(CPPFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)✓ $(NAME) pronto!$(RESET)"

clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(YELLOW)✗ Objetos limpos!$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)✗ Tudo limpo!$(RESET)"

re: fclean all

.PHONY: all clean fclean re mlx_download

