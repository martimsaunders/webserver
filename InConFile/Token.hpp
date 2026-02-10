/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Token.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 15:38:54 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/10 15:57:52 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <iostream>

enum TokenType{
	Identifier,
	String,
	Number,
	LBrace,
	RBrace,
	Semicolon,
	End,
};

struct Token{
	TokenType type;
	std::string value;
	int line;
	int col;
};

#endif