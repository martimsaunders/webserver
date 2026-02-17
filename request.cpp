/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateferr <mateferr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/12 14:56:33 by mateferr          #+#    #+#             */
/*   Updated: 2026/02/16 15:23:01 by mateferr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#define NEWLINE "#$"
#define HEADEND "#$#$"
#include <iostream>
#include <map>
#include <vector>
#include <cstdlib>

enum ParseStatus { Incomplete, Complete, Error };

enum RequestState {
  ReadingStartLineAndHeaders,
  ReadingContentLengthBody,
  ReadingChunkedBody
};

typedef struct s_HttpRequest {
  std::string method;
  std::string target; 
  std::string version; 
  std::map<std::string, std::string> headers;  
  size_t bodySize;
  std::string body;
  RequestState state;
  ParseStatus status;
} t_HttpRequest;

std::vector<std::string> split(std::string str, const std::string& c){
    std::vector<std::string> split;
    size_t pos = str.find(c);
    
    if(pos == std::string::npos){
        throw std::runtime_error("Split [" + str + "] with [" + c + "] fail");
    }
    
    while(pos != std::string::npos){
        split.push_back(str.substr(0, pos));
        str.erase(0, pos + c.size());
        pos = str.find(c);
    }
    split.push_back(str);
    
    return split;
}


void printCheck(std::vector<std::string>& vec){
    std::cout << "vector:" << std::endl;
    for(std::vector<std::string>::iterator it = vec.begin(); it != vec.end(); it++){
        std::cout << *it << std::endl;
    }
    exit(0);
}

void readStartLineAndHeaders(std::string& buffer, t_HttpRequest& request){
    size_t pos = buffer.find(HEADEND);
    if(pos == std::string::npos)
        return;
        
    std::string head = buffer.substr(0, pos);
    std::vector<std::string> headers = split(head, NEWLINE);

    std::vector<std::string> headerStart = split(headers.front(), " ");
    if(headerStart.size() != 3)
        throw std::runtime_error("Request start line format error: " + headers.front());

    request.method = headerStart[0];
    request.target = headerStart[1];
    request.version = headerStart[2];

    for(std::vector<std::string>::iterator it = headers.begin() + 1; it != headers.end(); it++){
        std::vector<std::string> header = split(*it, " ");
        if (header.size() != 2)
            throw std::runtime_error("Request header lines format error: " + *it);
        
        request.headers[header[0]] = header[1];
    }
    
    buffer.erase(0, pos + 4);
    if(request.method == "POST"){
        if(request.headers.find("Content-Length:") != request.headers.end()){
            request.state = ReadingContentLengthBody;
            request.bodySize = static_cast<size_t>(atoi(request.headers.find("Content-Length:")->second.c_str()));
        }
        else
            request.state = ReadingChunkedBody;
    }
    else
        request.status = Complete;
}

void parseRequest(std::string& buffer, t_HttpRequest& request){
       
    if (request.state == ReadingStartLineAndHeaders && request.status == Incomplete){
        readStartLineAndHeaders(buffer, request);
    }
    if(request.state == ReadingContentLengthBody && request.status == Incomplete){
        if(buffer.size() < request.bodySize)
            return;
        request.body = buffer.substr(0, request.bodySize);
        buffer.erase(0, request.bodySize);
        request.status = Complete;
    }
}

int main(){
    t_HttpRequest request;
    request.state = ReadingStartLineAndHeaders;
    request.status = Incomplete;
    std::string buffer;

    try{
        while(std::getline(std::cin, buffer)){
                std::cin.clear();
            parseRequest(buffer, request);
            if(request.status == Complete){
                std::cout << "Method: " << request.method << std::endl;
                std::cout << "Target: " << request.target << std::endl;
                std::cout << "Version: " << request.version << std::endl;
                
                std::cout << "______Headers_____" << std::endl;
                for(std::map<std::string, std::string>::iterator it = request.headers.begin(); it != request.headers.end(); it++){
                    std::cout << it->first << " " << it->second << std::endl;
                }
            }
        }
    }catch(std::exception& e){
        std::cout << e.what() << std::endl;
    }
}

/*
testar projetos webserver para ver comportamento do buffer de recv
printar requests para saber formato
*/