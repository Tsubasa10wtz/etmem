/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * etmem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 * http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: louhongxiang
 * Create: 2019-12-10
 * Description: This is a header file of the data structure definition for page.
 ******************************************************************************/

#ifndef ETMEMD_H
#define ETMEMD_H

#include <stdint.h>
#include <stdbool.h>

/*
 * page type specified by size
 * */
enum page_type {
    PTE_TYPE = 0,
    PMD_TYPE,
    PUD_TYPE,
    PAGE_TYPE_INVAL,
};

/*
 * page struct after scan and parse
 * */
struct page_refs {
    uint64_t addr;              /* page address */
    int count;                  /* page count */
    enum page_type type;        /* page type including PTE/PMD/PUD */

    struct page_refs *next;     /* point to next page */
};

/* memory grade is the result that judged by policy function after pagerefs come into it,
 * every policy fucntion has its own rule to make the choice which page is hot grade or
 * the other grades */
struct memory_grade {
    struct page_refs *hot_pages;
    struct page_refs *cold_pages;
};

#endif
