#pragma once

void Unprotect(ptrdiff_t where, size_t count);

template <typename TYPE>
void WriteValue(ptrdiff_t where, TYPE value)
{
	Unprotect(where, sizeof(value));
	*reinterpret_cast<TYPE  *>(where) = value;
}

/* eof */
