// File author is Ítalo Lima Marconato Matias
//
// Created on April 23 of 2019, at 18:08 BRT
// Last edited on January 04 of 2020, at 17:51 BRT

#include <chicago/alloc.h>
#include <chicago/config.h>
#include <chicago/file.h>
#include <chicago/string.h>

static Boolean ConfIsWhiteSpace(Char c) {
	return c == ' ' || c == '\r' || c == '\t';																											// Whitespace: ' ', '\r' (carriage return), '\t' (tab)
}

static Boolean ConfIsIdentifier(Char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||																							// Identifier: 'a' to 'z', 'A' to 'Z', '/', '-', '_', '@' and '.'
			c == '/' || c == '-' || c == '_' || c == '@' ||
			c == '.';
}

static Boolean ConfIsNumber(Char c) {
	return c >= '0' && c <= '9';																														// Number: 0 to 9
}

static Boolean ConfIsAlnum(Char c) {
	return ConfIsIdentifier(c) || ConfIsNumber(c);																										// Identifier or Number
}

static UIntPtr ConfGetLength(PWChar buf, UIntPtr pos, UIntPtr len, UInt8 type, WChar ch) {
	UIntPtr i = 0;
	
	while (((pos + i) < len) && (type == 0 ? (buf[pos + i] != ch) : (type == 1 ? ConfIsAlnum(buf[pos + i]) : ConfIsNumber(buf[pos + i])))) {			// Just get the length of this string/number/identifier
		i++;
	}
	
	return i;
}

static PWChar ConfStringFromChar(Char c) {
	PWChar str = (PWChar)MemAllocate(2 * sizeof(WChar));																								// 1 char for the char and 1 for the terminator
	
	if (str != Null) {
		str[0] = c;																																		// Copy the character to the string
		str[1] = 0;
	}
	
	return str;
}

static Boolean ConfCompare(PWChar s1, PWChar s2) {
	if (StrGetLength(s1) != StrGetLength(s2)) {																											// Same length?
		return False;																																	// Nope
	}
	
	return StrCompare(s1, s2);																															// Compare
}

static Boolean ConfAcceptToken(PList toks, UIntPtr pos, PWChar value) {
	if (pos >= toks->length) {																															// Sanity check
		return False;
	}
	
	return ConfCompare((PWChar)ListGet(toks, pos), value);
}

static Boolean ConfExpectToken(PList toks, UIntPtr pos, PWChar value, Boolean enums, Boolean equals) {
	if (pos >= toks->length) {																															// Sanity check
		return False;
	}
	
	PWChar str = (PWChar)ListGet(toks, pos);																											// Get the token
	
	if (!enums && ConfIsNumber(str[0])) {																												// Accept numbers?
		return False;																																	// Nope
	}
	
	if ((StrGetLength(str) != StrGetLength(value)) && equals) {																							// Same length?
		return False;																																	// Nope
	}
	
	Boolean scmp = StrCompare(str, value);
	
	if (!scmp && equals) {																																// Equals?
		return False;																																	// Nope
	} else if (scmp && !equals) {
		return False;																																	// Nope
	}
	
	return True;
}

static PList ConfLexBuffer(PWChar buf, UIntPtr len) {
	PList toks = ListNew(True, False);																													// Alloc the token list
	
	if (toks == Null) {
		return Null;																																	// Out of memory (probably)
	}
	
	for (UIntPtr pos = 0; pos < len; pos++) {																											// Let's go!																											// Let's go!
		while (ConfIsWhiteSpace(buf[pos])) {																											// Consume the whitespaces
			pos++;
		}
		
		if (buf[pos] == ';') {																															// Comment?
			pos++;																																		// Yes, consume until we find a new line
			
			while ((pos < len) && (buf[pos] != '\n')) {
				pos++;
			}
		} else if (buf[pos] == '\n' || buf[pos] == '=' || buf[pos] == ',') {																			// Single character tokens?
			PWChar str = ConfStringFromChar(buf[pos]);																									// Yes, try to create the string from the character
			
			if (str == Null) {
				ListFree(toks);																															// Failed...
				return Null;
			}
			
			if (!ListAdd(toks, str)) {																													// Add this token
				ListFree(toks);																															// Failed
				return Null;
			}
		} else if (buf[pos] == '\'' || buf[pos] == '\"') {																								// String?
			WChar end = buf[pos++];																														// Yes, set the string end character
			UIntPtr length = ConfGetLength(buf, pos, len, 0, end);																						// Get the length
			
			if ((pos + length) >= len) {
				ListFree(toks);																															// Unterminated string...
				return Null;
			}
			
			PWChar str = (PWChar)MemAllocate((length + 1) * sizeof(WChar));																				// Allocate memory
			
			if (str == Null) {
				ListFree(toks);																															// Failed...
				return Null;
			}
			
			for (UIntPtr i = 0; i < length; i++) {																										// Copy the string
				str[i] = buf[pos++];
			}
			
			str[length] = 0;																															// End the string
			
			if (!ListAdd(toks, str)) {																													// Add this token
				ListFree(toks);																															// Failed
				return Null;
			}
		} else if (ConfIsIdentifier(buf[pos])) {																										// Identifier?
			UIntPtr length = ConfGetLength(buf, pos, len, 1, 0);																						// Yes, get the length
			PWChar str = (PWChar)MemAllocate((length + 1) * sizeof(WChar));																				// Allocate memory
			
			if (str == Null) {
				ListFree(toks);																															// Failed...
				return Null;
			}
			
			for (UIntPtr i = 0; i < length; i++) {																										// Copy the string
				str[i] = buf[pos++];
			}
			
			str[length] = 0;																															// End the string
			pos--;																																		// Go one character back
			
			if (!ListAdd(toks, str)) {																													// Add this token
				ListFree(toks);																															// Failed
				return Null;
			}
		} else if (ConfIsNumber(buf[pos])) {																											// Number?
			UIntPtr length = ConfGetLength(buf, pos, len, 2, 0);																						// Yes, get the length
			PWChar str = (PWChar)MemAllocate((length + 1) * sizeof(WChar));																				// Allocate memory
			
			if (str == Null) {
				ListFree(toks);																															// Failed...
				return Null;
			}
			
			for (UIntPtr i = 0; i < length; i++) {																										// Copy the string
				str[i] = buf[pos++];
			}
			
			str[length] = 0;																															// End the string
			pos--;																																		// Go one character back
			
			if (!ListAdd(toks, str)) {																													// Add this token
				ListFree(toks);																															// Failed
				return Null;
			}
		} else {
			ListFree(toks);																																// Invalid character...
			return Null;
		}
	}
	
	return toks;
}

static Boolean ConfParseTokens(PList conf, PList toks) {
	for (UIntPtr pos = 0; pos < toks->length;) {																										// Let's parse!
		while (ConfAcceptToken(toks, pos, L"\n")) {																										// Consume new lines
			pos++;
		}
		
		if (pos >= toks->length) {																														// End?
			return True;																																// Yes :)
		} else if (!ConfExpectToken(toks, pos, L"=", False, False)) {																					// We expect to find an identifier here
			return False;
		} else if (!ConfExpectToken(toks, pos, L",", False, False)) {
			return False;
		} else if (!ConfExpectToken(toks, pos, L"\n", False, False)) {
			return False;
		}
		
		PConfField field = (PConfField)MemAllocate(sizeof(ConfField));																					// Alloc space for the field
		
		if (field == Null) {
			return False;																																// Failed
		}
		
		field->name = StrDuplicate((PWChar)ListGet(toks, pos++));																						// Set the name
		
		if (field->name == Null) {
			MemFree((UIntPtr)field);
			return False;
		} else if (ConfAcceptToken(toks, pos, L"=")) {																									// Set the value?
			pos++;																																		// Yes
			
			if (!ConfExpectToken(toks, pos, L"=", True, False)) {																						// Accept almost anything here
				MemFree((UIntPtr)field->name);
				MemFree((UIntPtr)field);
				return False;
			} else if (!ConfExpectToken(toks, pos, L",", True, False)) {
				MemFree((UIntPtr)field->name);
				MemFree((UIntPtr)field);
				return False;
			} else if (!ConfExpectToken(toks, pos, L"\n", True, False)) {
				MemFree((UIntPtr)field->name);
				MemFree((UIntPtr)field);
				return False;
			}
			
			field->value = StrDuplicate((PWChar)ListGet(toks, pos++));																					// Set the value
		} else {
			field->value = StrDuplicate(L"1");																											// Just set the default value
		}
		
		if (field->value == Null) {
			MemFree((UIntPtr)field->name);
			MemFree((UIntPtr)field);
			return False;
		}
		
		if (!ConfAcceptToken(toks, pos, L"\n")) {																										// Set the attributes
			field->attrs = ListNew(False, False);																										// Yes, init the attrib list
			
			if (field->attrs == Null) {
				ConfFreeField(field);																													// Failed...
				return False;
			} else if (!ConfExpectToken(toks, pos++, L",", False, True)) {																				// Now we should have an comma here
				ConfFreeField(field);
				return False;
			}
			
start:		;PConfFieldAttribute attr = (PConfFieldAttribute)MemAllocate(sizeof(ConfFieldAttribute));													// Alloc the attr
			
			if (attr == Null) {
				ConfFreeField(field);																													// Failed
				return False;
			} else if (!ConfExpectToken(toks, pos, L"=", True, False)) {																				// Accept almost anything here
				MemFree((UIntPtr)attr);
				ConfFreeField(field);
				return False;
			} else if (!ConfExpectToken(toks, pos, L",", True, False)) {
				MemFree((UIntPtr)attr);
				ConfFreeField(field);
				return False;
			} else if (!ConfExpectToken(toks, pos, L"\n", True, False)) {																				// Accept almost anything here
				MemFree((UIntPtr)attr);
				ConfFreeField(field);
				return False;
			}
			
			attr->name = StrDuplicate((PWChar)ListGet(toks, pos++));																					// Get the name
			
			if (attr->name == Null) {
				MemFree((UIntPtr)attr);
				ConfFreeField(field);
				return False;
			} else if (ConfAcceptToken(toks, pos, L"=")) {																								// Set the value?
				pos++;																																	// Yes

				if (!ConfExpectToken(toks, pos, L"=", True, False)) {																					// Accept almost anything here
					MemFree((UIntPtr)attr->name);
					MemFree((UIntPtr)attr);
					ConfFreeField(field);
					return False;
				} else if (!ConfExpectToken(toks, pos, L",", True, False)) {
					MemFree((UIntPtr)attr->name);
					MemFree((UIntPtr)attr);
					ConfFreeField(field);
					return False;
				} else if (!ConfExpectToken(toks, pos, L"\n", True, False)) {
					MemFree((UIntPtr)attr->name);
					MemFree((UIntPtr)attr);
					ConfFreeField(field);
					return False;
				}

				attr->value = StrDuplicate((PWChar)ListGet(toks, pos++));																				// Set the value
			} else {
				attr->value = StrDuplicate(L"1");																										// Just set the default value
			}
			
			if (attr->value == Null) {
				MemFree((UIntPtr)attr->name);
				MemFree((UIntPtr)attr);
				ConfFreeField(field);
				return False;
			} else if (!ListAdd(field->attrs, attr)) {																									// Add the attr
				MemFree((UIntPtr)attr->value);
				MemFree((UIntPtr)attr->name);
				MemFree((UIntPtr)attr);
				ConfFreeField(field);
				return False;
			} else if (ConfAcceptToken(toks, pos, L",")) {																								// More attrs?
				pos++;
				goto start;																																// Yes
			}
		} else {
			field->attrs = Null;																														// No attributes
		}
		
		if (!ConfExpectToken(toks, pos, L"\n", False, True) && pos < toks->length) {																	// Expect new line
			ConfFreeField(field);
			return False;
		} else if (!ListAdd(conf, field)) {																												// Add the field!
			ConfFreeField(field);
			return False;
		}
	}
	
	return True;
}

Void ConfFreeField(PConfField field) {
	if (field != Null) {																																// Sanity check
		MemFree((UIntPtr)field->name);																													// Free the name
		MemFree((UIntPtr)field->value);																													// Free the value
		
		if (field->attrs != Null) {																														// Free the attributes?
			while (field->attrs->length != 0) {																											// Yes!
				PConfFieldAttribute attr = ListRemove(field->attrs, field->attrs->length - 1);
				
				MemFree((UIntPtr)attr->value);																											// Free the value
				MemFree((UIntPtr)attr->name);																											// Free the name
				MemFree((UIntPtr)attr);																													// And free the attr itself
			}
			
			ListFree(field->attrs);																														// Free the attr list
		}
		
		MemFree((UIntPtr)field);																														// And the field struct
	}
}

Void ConfFree(PList conf) {
	if (conf != Null) {																																	// Sanity check
		while (conf->length != 0) {																														// Free all the fields!
			ConfFreeField(ListRemove(conf, conf->length - 1));
		}
		
		ListFree(conf);																																	// Free the conf struct
	}
}

PList ConfLoad(PWChar name) {
	if (name == Null) {																																	// Sanity check
		return Null;
	}
	
	PList conf = ListNew(False, False);																													// Create the output list
	
	if (conf == Null) {
		return Null;																																	// Failed :(
	}
	
	PWChar path = name[0] == '/' ? StrDuplicate(name) : FsJoinPath(L"/System/Configurations", name);													// Create the path string
	
	if (path == Null) {
		ListFree(conf);																																	// Failed :(
		return Null;
	}
	
	PFsNode file = FsOpenFile(path);																													// Try to open the config file
	
	MemFree((UIntPtr)path);																																// Free the path, we don't need it anymore
	
	if (file == Null) {
		ListFree(conf);																																	// Failed to open it :(
		return Null;
	}
	
	PChar bufa = (PChar)MemAllocate(file->length + 1);																									// Alloc space for reading the file
	
	if (bufa == Null) {
		FsCloseFile(file);																																// We're probably out of memory
		ListFree(conf);
		return Null;
	} else if (FsReadFile(file, 0, file->length, (PUInt8)bufa) != file->length) {																		// Read the file!
		FsCloseFile(file);																																// Failed...?
		ListFree(conf);
		return Null;
	}
	
	if (bufa[file->length] != 0) {
		bufa[file->length] = 0;																															// NUL terminator in the end
	}
	
	PWChar buf = (PWChar)MemAllocate((file->length + 1) * sizeof(WChar));																				// Alloc space for the wchar str
	
	FsCloseFile(file);																																	// Close the file, we don't need it anymore
	
	if (buf == Null) {
		MemFree((UIntPtr)bufa);																															// Failed
		ListFree(conf);
		return Null;
	}
	
	StrUnicodeFromC(buf, bufa, StrGetLengthC(bufa));																									// Convert to unicode
	MemFree((UIntPtr)bufa);
	
	PList toks = ConfLexBuffer(buf, StrGetLength(buf));																									// Lex!
	
	if (toks == Null) {
		MemFree((UIntPtr)buf);																															// Failed
		ListFree(conf);
		return Null;
	}
	
	MemFree((UIntPtr)buf);																																// Finally free the buffer!
	
	if (!ConfParseTokens(conf, toks)) {																													// Parse!
		MemFree((UIntPtr)buf);
		ConfFree(conf);																																	// ...
		ListFree(toks);
		return Null;
	}
	
	ListFree(toks);																																		// Free the token list
	
	return conf;
}

PConfField ConfGetField(PList conf, PWChar name) {
	if (conf == Null || name == Null) {																													// Sanity check
		return Null;
	}
	
	ListForeach(conf, i) {																																// Search for the field with name == 'name'
		PConfField field = (PConfField)i->data;
		
		if (StrCompare(field->name, name)) {																											// Found?
			return field;																																// Yes!
		}
	}
	
	return Null;
}

PConfFieldAttribute ConfGetFieldAttribute(PConfField field, PWChar name) {
	if (field == Null || name == Null || field->attrs == Null) {																						// Sanity check
		return Null;
	}
	
	ListForeach(field->attrs, i) {																														// Search for the field with name == 'name'
		PConfFieldAttribute attr = (PConfFieldAttribute)i->data;
		
		if (StrCompare(attr->name, name)) {																												// Found?
			return attr;																																// Yes!
		}
	}
	
	return Null;
}
