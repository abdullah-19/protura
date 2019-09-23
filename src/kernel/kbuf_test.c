/*
 * Copyright (C) 2019 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
/*
 * Tests for kbuf.c - included directly at the end of kbuf.c
 */

#include <protura/types.h>
#include <protura/kbuf.h>
#include <protura/ktest.h>

static void kbuf_multiread_test(const struct ktest_unit *unit, struct ktest *kt)
{
    int read_size = unit->arg;
    struct page *tmp_page = palloc(2, PAL_KERNEL);
    struct kbuf kbuf;
    kbuf_init(&kbuf);
    kbuf_add_page(&kbuf);
    kbuf_add_page(&kbuf);
    kbuf_add_page(&kbuf);
    kbuf_add_page(&kbuf);

    memset(tmp_page->virt, 0, PG_SIZE * 4);

    struct page *pages[4];
    pages[0] = list_first_entry(&kbuf.pages, struct page, page_list_node);
    pages[1] = list_next_entry(pages[0], page_list_node);
    pages[2] = list_next_entry(pages[1], page_list_node);
    pages[3] = list_next_entry(pages[2], page_list_node);

    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &pages[0]->page_list_node));
    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &pages[1]->page_list_node));
    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &pages[2]->page_list_node));
    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &pages[3]->page_list_node));

    /*
     * We write uint16_t values and do some math to try to reduce patterns in
     * buffer.
     */
    int i, k;
    for (k = 0; k < 4; k++)
        for (i = 0; i < PG_SIZE / 2; i++)
            ((uint16_t *)pages[k]->virt)[i] = ((i + k * PG_SIZE / 2) * 7) & 0xFFFF;

    kbuf.cur_pos.offset = PG_SIZE * 4;
    int total_reads = PG_SIZE * 4 / read_size;

    for (i = 0; i < total_reads; i++) {
        int err = kbuf_read(&kbuf, i * read_size, tmp_page->virt + i * read_size, read_size);
        ktest_assert_equal(kt, read_size, err);

        size_t total_read_len = (i + 1) * read_size;
        int pages_len[4];
        pages_len[0] = (total_read_len > PG_SIZE)? PG_SIZE: total_read_len;
        pages_len[1] = (total_read_len > PG_SIZE * 2)? PG_SIZE: total_read_len - PG_SIZE;
        pages_len[2] = (total_read_len > PG_SIZE * 3)? PG_SIZE: total_read_len - PG_SIZE * 2;
        pages_len[3] = (total_read_len > PG_SIZE * 4)? PG_SIZE: total_read_len - PG_SIZE * 3;

        for (k = 0; k < 4; k++)
            if (pages_len[k] > 0)
                ktest_assert_equal_mem(kt, pages[k]->virt, tmp_page->virt + k * PG_SIZE, pages_len[k]);
    }

    /* if our read_size doesn't divide the kbuf size evenly, then read the
     * last of the buffer and verify the partial read happened correctly */
    if ((PG_SIZE * 4) % read_size) {
        int left_over = (PG_SIZE * 4) % read_size;

        int err = kbuf_read(&kbuf, total_reads * read_size, tmp_page->virt + total_reads * read_size, read_size);
        ktest_assert_equal(kt, left_over, err);
    }

    /* At this point, the whole buffer should be written, so verify the whole thing */
    for (k = 0; k < 4; k++)
        ktest_assert_equal_mem(kt, pages[k]->virt, tmp_page->virt + k * PG_SIZE, PG_SIZE);

    kbuf_clear(&kbuf);
    pfree(tmp_page, 2);
}


static void kbuf_read_from_offset_test(const struct ktest_unit *unit, struct ktest *kt)
{
    struct page *tmp_page = palloc(0, PAL_KERNEL);
    struct kbuf kbuf;
    kbuf_init(&kbuf);
    kbuf_add_page(&kbuf);

    struct page *cur_page = list_first_entry(&kbuf.pages, struct page, page_list_node);
    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &cur_page->page_list_node));

    int i;
    for(i = 0; i < PG_SIZE / 2; i++)
        ((uint16_t *)tmp_page->virt)[i] = (i * 7) & 0xFFFF;

    kbuf.cur_pos.offset = PG_SIZE;

    for (i = 0; i < PG_SIZE - unit->arg; i++) {
        int err = kbuf_read(&kbuf, i, tmp_page->virt, unit->arg);
        ktest_assert_equal(kt, unit->arg, err);

        ktest_assert_equal_mem(kt, cur_page->virt + i, tmp_page->virt, unit->arg);
    }

    kbuf_clear(&kbuf);
    pfree(tmp_page, 0);
}

static void kbuf_read_from_start_test(const struct ktest_unit *unit, struct ktest *kt)
{
    struct page *tmp_page = palloc(0, PAL_KERNEL);
    struct kbuf kbuf;
    kbuf_init(&kbuf);
    kbuf_add_page(&kbuf);

    struct page *cur_page = list_first_entry(&kbuf.pages, struct page, page_list_node);
    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &cur_page->page_list_node));

    int i;
    for(i = 0; i < PG_SIZE / 2; i++)
        ((uint16_t *)tmp_page->virt)[i] = (i * 7) & 0xFFFF;

    kbuf.cur_pos.offset = PG_SIZE;

    memset(tmp_page->virt, 0, PG_SIZE);
    int err = kbuf_read(&kbuf, 0, tmp_page->virt, unit->arg);
    ktest_assert_equal(kt, unit->arg, err);

    ktest_assert_equal_mem(kt, cur_page->virt, tmp_page->virt, unit->arg);

    kbuf_clear(&kbuf);
    pfree(tmp_page, 0);
}

static void kbuf_read_past_the_end_test(const struct ktest_unit *unit, struct ktest *kt)
{
    struct page *tmp_page = palloc(0, PAL_KERNEL);
    struct kbuf kbuf;
    kbuf_init(&kbuf);
    kbuf_add_page(&kbuf);

    struct page *cur_page = list_first_entry(&kbuf.pages, struct page, page_list_node);
    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &cur_page->page_list_node));

    int i;
    for(i = 0; i < PG_SIZE / 2; i++)
        ((uint16_t *)tmp_page->virt)[i] = (i * 7) & 0xFFFF;

    kbuf.cur_pos.offset = unit->arg;

    memset(tmp_page->virt, 0, PG_SIZE);
    int err = kbuf_read(&kbuf, 0, tmp_page->virt, PG_SIZE);
    ktest_assert_equal(kt, unit->arg, err);

    ktest_assert_equal_mem(kt, cur_page->virt, tmp_page->virt, unit->arg);

    kbuf_clear(&kbuf);
    pfree(tmp_page, 0);
}

static void kbuf_multiwrite_test(const struct ktest_unit *unit, struct ktest *kt)
{
    int k;
    int write_size = unit->arg;
    struct page *tmp_page = palloc(2, PAL_KERNEL);
    struct kbuf kbuf;
    kbuf_init(&kbuf);
    kbuf_add_page(&kbuf);
    kbuf_add_page(&kbuf);
    kbuf_add_page(&kbuf);
    kbuf_add_page(&kbuf);

    struct page *pages[4];
    pages[0] = list_first_entry(&kbuf.pages, struct page, page_list_node);
    pages[1] = list_next_entry(pages[0], page_list_node);
    pages[2] = list_next_entry(pages[1], page_list_node);
    pages[3] = list_next_entry(pages[2], page_list_node);

    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &pages[0]->page_list_node));
    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &pages[1]->page_list_node));
    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &pages[2]->page_list_node));
    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &pages[3]->page_list_node));

    /*
     * We write uint16_t values and do some math to try to reduce patterns in
     * buffer.
     */
    int i;
    for(i = 0; i < PG_SIZE * 2; i++)
        ((uint16_t *)tmp_page->virt)[i] = (i * 7) & 0xFFFF;

    int total_writes = PG_SIZE * 4 / write_size;

    for (i = 0; i < total_writes; i++) {
        int err = kbuf_write(&kbuf, tmp_page->virt + i * write_size, write_size);
        ktest_assert_equal(kt, write_size, err);

        size_t total_write_len = (i + 1) * write_size;
        int pages_len[4];
        pages_len[0] = (total_write_len > PG_SIZE)? PG_SIZE: total_write_len;
        pages_len[1] = (total_write_len > PG_SIZE * 2)? PG_SIZE: total_write_len - PG_SIZE;
        pages_len[2] = (total_write_len > PG_SIZE * 3)? PG_SIZE: total_write_len - PG_SIZE * 2;
        pages_len[3] = (total_write_len > PG_SIZE * 4)? PG_SIZE: total_write_len - PG_SIZE * 3;

        for (k = 0; k < 4; k++)
            if (pages_len[k] > 0)
                ktest_assert_equal_mem(kt, pages[k]->virt, tmp_page->virt + k * PG_SIZE, pages_len[k]);
    }

    /* if our write_size doesn't divide the kbuf size evenly, then write the
     * last of the buffer and verify the partial write happened correctly */
    if ((PG_SIZE * 4) % write_size) {
        int left_over = (PG_SIZE * 4) % write_size;

        int err = kbuf_write(&kbuf, tmp_page->virt + total_writes * write_size, write_size);
        ktest_assert_equal(kt, left_over, err);
    }

    /* At this point, the whole buffer should be written, so verify the whole thing */
    for (k = 0; k < 4; k++)
        ktest_assert_equal_mem(kt, pages[k]->virt, tmp_page->virt + k * PG_SIZE, PG_SIZE);

    kbuf_clear(&kbuf);
    pfree(tmp_page, 2);
}

static void kbuf_write_two_page_test(const struct ktest_unit *unit, struct ktest *kt)
{
    struct page *tmp_page = palloc(1, PAL_KERNEL);
    struct kbuf kbuf;
    kbuf_init(&kbuf);
    kbuf_add_page(&kbuf);
    kbuf_add_page(&kbuf);

    int i;
    for(i = 0; i < PG_SIZE * 2; i++)
        ((char *)tmp_page->virt)[i] = i % 256;

    int err = kbuf_write(&kbuf, tmp_page->virt, unit->arg + PG_SIZE);
    ktest_assert_equal(kt, unit->arg + PG_SIZE, err);

    struct page *cur_page = list_first_entry(&kbuf.pages, struct page, page_list_node);
    struct page *next_page = list_next_entry(cur_page, page_list_node);

    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &cur_page->page_list_node));
    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &next_page->page_list_node));

    ktest_assert_equal_mem(kt, cur_page->virt, tmp_page->virt, PG_SIZE);
    ktest_assert_equal_mem(kt, next_page->virt, tmp_page->virt, unit->arg);

    kbuf_clear(&kbuf);
    pfree(tmp_page, 0);
}

static void kbuf_write_one_page_test(const struct ktest_unit *unit, struct ktest *kt)
{
    struct page *tmp_page = palloc(0, PAL_KERNEL);
    struct kbuf kbuf;
    kbuf_init(&kbuf);
    kbuf_add_page(&kbuf);

    int i;
    for(i = 0; i < PG_SIZE; i++)
        ((char *)tmp_page->virt)[i] = i % 256;

    int err = kbuf_write(&kbuf, tmp_page->virt, unit->arg);
    ktest_assert_equal(kt, unit->arg, err);

    struct page *cur_page = list_first_entry(&kbuf.pages, struct page, page_list_node);

    ktest_assert_equal(kt, 0, list_ptr_is_head(&kbuf.pages, &cur_page->page_list_node));
    ktest_assert_equal_mem(kt, cur_page->virt, tmp_page->virt, unit->arg);

    kbuf_clear(&kbuf);
    pfree(tmp_page, 0);
}

static void kbuf_add_page_test(const struct ktest_unit *unit, struct ktest *kt)
{
    struct kbuf kbuf;
    kbuf_init(&kbuf);

    kbuf_add_page(&kbuf);

    ktest_assert_equal(kt, 1, !list_empty(&kbuf.pages));
    ktest_assert_equal(kt, 1, kbuf.page_count);
    ktest_assert_equal(kt, 0, kbuf.cur_pos.offset);

    kbuf_add_page(&kbuf);

    ktest_assert_equal(kt, 1, !list_empty(&kbuf.pages));
    ktest_assert_equal(kt, 2, kbuf.page_count);
    ktest_assert_equal(kt, 0, kbuf.cur_pos.offset);

    kbuf_clear(&kbuf);

    ktest_assert_equal(kt, 1, list_empty(&kbuf.pages));
    ktest_assert_equal(kt, 0, kbuf.page_count);
    ktest_assert_equal(kt, 0, kbuf.cur_pos.offset);
}

static void kbuf_init_test(const struct ktest_unit *unit, struct ktest *kt)
{
    struct kbuf kbuf;
    kbuf_init(&kbuf);

    ktest_assert_equal(kt, 1, list_empty(&kbuf.pages));
    ktest_assert_equal(kt, 0, kbuf.page_count);
    ktest_assert_equal(kt, 0, kbuf.cur_pos.offset);

    kbuf_clear(&kbuf);
}

static const struct ktest_unit kbuf_test_units[] = {
    KTEST_UNIT_INIT("kbuf-init-test", kbuf_init_test),
    KTEST_UNIT_INIT("kbuf-add-page", kbuf_add_page_test),
    KTEST_UNIT_INIT_ARG("kbuf-write-one-page-test", kbuf_write_one_page_test, 0),
    KTEST_UNIT_INIT_ARG("kbuf-write-one-page-test", kbuf_write_one_page_test, 1),
    KTEST_UNIT_INIT_ARG("kbuf-write-one-page-test", kbuf_write_one_page_test, 256),
    KTEST_UNIT_INIT_ARG("kbuf-write-one-page-test", kbuf_write_one_page_test, PG_SIZE),
    KTEST_UNIT_INIT_ARG("kbuf-write-two-page-test", kbuf_write_two_page_test, 0),
    KTEST_UNIT_INIT_ARG("kbuf-write-two-page-test", kbuf_write_two_page_test, 1),
    KTEST_UNIT_INIT_ARG("kbuf-write-two-page-test", kbuf_write_two_page_test, 256),
    KTEST_UNIT_INIT_ARG("kbuf-write-two-page-test", kbuf_write_two_page_test, PG_SIZE),
    KTEST_UNIT_INIT_ARG("kbuf-multi-write-test", kbuf_multiwrite_test, 32),
    KTEST_UNIT_INIT_ARG("kbuf-multi-write-test", kbuf_multiwrite_test, 64),
    KTEST_UNIT_INIT_ARG("kbuf-multi-write-test", kbuf_multiwrite_test, 256),
    KTEST_UNIT_INIT_ARG("kbuf-multi-write-test", kbuf_multiwrite_test, PG_SIZE),
    KTEST_UNIT_INIT_ARG("kbuf-multi-write-test", kbuf_multiwrite_test, PG_SIZE * 2),
    KTEST_UNIT_INIT_ARG("kbuf-multi-write-test", kbuf_multiwrite_test, PG_SIZE * 4),
    KTEST_UNIT_INIT_ARG("kbuf-multi-write-test", kbuf_multiwrite_test, 10),
    KTEST_UNIT_INIT_ARG("kbuf-multi-write-test", kbuf_multiwrite_test, 100),
    KTEST_UNIT_INIT_ARG("kbuf-multi-write-test", kbuf_multiwrite_test, 1000),
    KTEST_UNIT_INIT_ARG("kbuf-multi-write-test", kbuf_multiwrite_test, 10000),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-start-test", kbuf_read_from_start_test, 0),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-start-test", kbuf_read_from_start_test, 1),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-start-test", kbuf_read_from_start_test, 20),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-start-test", kbuf_read_from_start_test, 256),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-start-test", kbuf_read_from_start_test, 2048),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-start-test", kbuf_read_from_start_test, 3000),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-start-test", kbuf_read_from_start_test, PG_SIZE),
    KTEST_UNIT_INIT_ARG("kbuf-read-past-the-end-test", kbuf_read_past_the_end_test, 0),
    KTEST_UNIT_INIT_ARG("kbuf-read-past-the-end-test", kbuf_read_past_the_end_test, 1),
    KTEST_UNIT_INIT_ARG("kbuf-read-past-the-end-test", kbuf_read_past_the_end_test, 20),
    KTEST_UNIT_INIT_ARG("kbuf-read-past-the-end-test", kbuf_read_past_the_end_test, 256),
    KTEST_UNIT_INIT_ARG("kbuf-read-past-the-end-test", kbuf_read_past_the_end_test, 2048),
    KTEST_UNIT_INIT_ARG("kbuf-read-past-the-end-test", kbuf_read_past_the_end_test, 3000),
    KTEST_UNIT_INIT_ARG("kbuf-read-past-the-end-test", kbuf_read_past_the_end_test, PG_SIZE - 1),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-offset-test", kbuf_read_from_offset_test, 1),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-offset-test", kbuf_read_from_offset_test, 20),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-offset-test", kbuf_read_from_offset_test, 256),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-offset-test", kbuf_read_from_offset_test, 2048),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-offset-test", kbuf_read_from_offset_test, 3000),
    KTEST_UNIT_INIT_ARG("kbuf-read-from-offset-test", kbuf_read_from_offset_test, PG_SIZE - 1),
    KTEST_UNIT_INIT_ARG("kbuf-multi-read-test", kbuf_multiread_test, 32),
    KTEST_UNIT_INIT_ARG("kbuf-multi-read-test", kbuf_multiread_test, 64),
    KTEST_UNIT_INIT_ARG("kbuf-multi-read-test", kbuf_multiread_test, 256),
    KTEST_UNIT_INIT_ARG("kbuf-multi-read-test", kbuf_multiread_test, PG_SIZE),
    KTEST_UNIT_INIT_ARG("kbuf-multi-read-test", kbuf_multiread_test, PG_SIZE * 2),
    KTEST_UNIT_INIT_ARG("kbuf-multi-read-test", kbuf_multiread_test, PG_SIZE * 4),
    KTEST_UNIT_INIT_ARG("kbuf-multi-read-test", kbuf_multiread_test, 10),
    KTEST_UNIT_INIT_ARG("kbuf-multi-read-test", kbuf_multiread_test, 100),
    KTEST_UNIT_INIT_ARG("kbuf-multi-read-test", kbuf_multiread_test, 1000),
    KTEST_UNIT_INIT_ARG("kbuf-multi-read-test", kbuf_multiread_test, 10000),
};

static const struct ktest_module __ktest kbuf_test_module = {
    .name = "kbuf",
    .tests = kbuf_test_units,
    .test_count = ARRAY_SIZE(kbuf_test_units),
};
