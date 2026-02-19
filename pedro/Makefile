CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
SRCS = src/main.cpp \
       src/HttpRequest.cpp \
       src/ServerConfig.cpp \
       src/Location.cpp \
       src/RequestHandler.cpp \
       src/ResponseBuilder.cpp \
       src/FileService.cpp \
       src/HttpResponse.cpp \
	   src/Mime.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = webserv_test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(TARGET)

re: fclean all

.PHONY: all clean fclean re
