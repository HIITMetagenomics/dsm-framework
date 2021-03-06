/* alphabet_mapper_cont.h
 * Copyright (C) 2008, Francisco Claude, all rights reserved.
 *
 * alphabet_mapper_cont definition
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef _ALPHABET_MAPPER_CONT_H
#define _ALPHABET_MAPPER_CONT_H

#include <basics.h>
#include <iostream>
#include <alphabet_mapper.h>
#include <static_bitsequence.h>
#include <static_bitsequence_builder.h>

using namespace std;

/** Mapper that doesn't change the value (identity)
 * 
 *  @author Francisco Claude
 */
class alphabet_mapper_cont : public alphabet_mapper {
  public:
    alphabet_mapper_cont(uint * seq, uint n, static_bitsequence_builder *bmb);
    virtual ~alphabet_mapper_cont();
    virtual uint map(uint s);
    virtual uint unmap(uint s);
    virtual uint size();
    virtual uint save(FILE *fp);
    static alphabet_mapper_cont * load(FILE *fp);
    
  protected:
    alphabet_mapper_cont();
    static_bitsequence * m;
};

#endif /* _ALPHABET_MAPPER_CONT_H */
