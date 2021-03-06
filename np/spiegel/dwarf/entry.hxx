/*
 * Copyright 2011-2012 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __np_spiegel_dwarf_entry_hxx__
#define __np_spiegel_dwarf_entry_hxx__ 1

#include "np/spiegel/common.hxx"
#include "value.hxx"
#include "abbrev.hxx"
#include "enumerations.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

class abbrev_t;

class entry_t
{
public:
    entry_t()
     :  offset_(0),
	level_(0),
	abbrev_(0),
	nvalues_(0)
    {
	// invalidate the by-attribute indexes
	memset(byattr_basic_, 0xff, sizeof(byattr_basic_));
	memset(byattr_user_, 0xff, sizeof(byattr_user_));
    }

    void setup(size_t offset, unsigned level, const abbrev_t *a);
    void partial_setup(const entry_t &o)
    {
	offset_ = o.offset_;
	level_ = o.level_;
	abbrev_ = 0;
    }
    void partial_setup(uint32_t off, uint32_t lev)
    {
	offset_ = off;
	level_ = lev;
	abbrev_ = 0;
    }

    void add_attribute(uint32_t name, value_t val)
    {
	assert(nvalues_ < MAX_VALUES);
	values_[nvalues_] = val;
	if (name < DW_AT_max_basic)
	    byattr_basic_[name] = nvalues_;
	else if (name >= DW_AT_lo_user && name < DW_AT_max_user)
	    byattr_user_[name-DW_AT_lo_user] = nvalues_;
	else
	    return;	// silently ignore this
	nvalues_++;
    }

    unsigned get_offset() const { return offset_; }
    unsigned get_level() const { return level_; }
    const abbrev_t *get_abbrev() const { return abbrev_; }
    uint32_t get_tag() const { return abbrev_->tag; }
    bool has_children() const { return abbrev_->children; }
    void dump() const;

    const value_t *get_attribute(uint32_t name) const
    {
	int8_t n = -1;
	if (name < DW_AT_max_basic)
	    n = byattr_basic_[name];
	else if (name >= DW_AT_lo_user && name < DW_AT_max_user)
	    n = byattr_user_[name-DW_AT_lo_user];
	return (n < 0 ? 0 : &values_[n]);
    }
    const char *get_string_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	return (v && v->type == value_t::T_STRING ? v->val.string : 0);
    }
    uint32_t get_uint32_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	if (!v)
	    return 0;
	else if (v->type == value_t::T_UINT32)
	    return v->val.uint32;
	else
	    return 0;
    }
    uint64_t get_uint64_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	if (!v)
	    return 0;
	else if (v->type == value_t::T_UINT32)
	    return v->val.uint32;
	else if (v->type == value_t::T_UINT64)
	    return v->val.uint64;
	else
	    return 0;
    }
    addr_t get_address_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	if (!v)
	    return 0;
	else if (v->type == value_t::T_UINT32)
	    return (addr_t)v->val.uint32;
	else if (v->type == value_t::T_UINT64)
	    return (addr_t)v->val.uint64;
	else
	    return 0;
    }
    const reference_t get_reference_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	return (v && v->type == value_t::T_REF ? v->val.ref : reference_t::null);
    }

private:
    enum
    {
	MAX_VALUES = 32,	// the most defined attributes for any tag
				// in DWARF2 is 23 for DW_TAG_subprogram
    };
    unsigned offset_;
    unsigned level_;
    const abbrev_t *abbrev_;
    uint8_t nvalues_;
    value_t values_[MAX_VALUES];
    // by-attribute indexes into values_[], -1=invalid
    int8_t byattr_basic_[DW_AT_max_basic];
    int8_t byattr_user_[DW_AT_max_user-DW_AT_lo_user];
};


// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_entry_hxx__
