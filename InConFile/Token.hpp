/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Token.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 15:38:54 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/11 13:09:55 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TOKEN_HPP
# define TOKEN_HPP

# include <iostream>
# include <string>

enum			TokenType
{
	Identifier,
	String,
	Number,
	LBrace,
	RBrace,
	Semicolon,
	End,
};

struct			Token
{
	int			col;
	int			line;
	TokenType	type;
	std::string value;
};

std::vector<Token> tokenize(std::string const &text);

#endif