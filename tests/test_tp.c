/* -*- coding: utf-8 -*-
 ###############################################################################
 # Author: Pierre Vigneras <pierre.vigneras@bull.net>
 # Created on: Oct 2, 2014
 # Contributors:
 ###############################################################################
 # Copyright (C) 2012  Bull S. A. S.  -  All rights reserved
 # Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
 # This is not Free or Open Source software.
 # Please contact Bull S. A. S. for details about its license.
 ###############################################################################
 */


struct mock_s {
    bxitp_topo_p topo;
    bxitp_asic_p asic;
    bxitp_port_p port;
    bxitp_error_e rc;
    const char * errmsg;
    const char * errline;
    uint64_t errline_nb;
    uint32_t total_line_nb;
};

bxitp_error_e topo_cb(bxitp_topo_p topo, void * data, const char * line, uint32_t line_nb) {
    CU_ASSERT_PTR_NOT_NULL_FATAL(topo);
    CU_ASSERT_PTR_NOT_NULL_FATAL(data);
    struct mock_s * const mock = (struct mock_s *) data;
    mock->topo = topo;
    mock->errline = line;
    mock->errline_nb = line_nb;
    DEBUG(TEST_LOGGER, "Called: mock.topo.family=%d", mock->topo->family);
    return BXITP_OK;
}

bxitp_error_e asic_cb(bxitp_asic_p asic, void * data, const char* line, uint32_t line_nb) {
    UNUSED(line);
    UNUSED(line_nb);
    CU_ASSERT_PTR_NOT_NULL_FATAL(asic);
    CU_ASSERT_PTR_NOT_NULL_FATAL(data);
    struct mock_s * const mock = (struct mock_s *) data;
    mock->asic = asic;
    DEBUG(TEST_LOGGER, "Called: mock.asic.radix=%d", mock->asic->radix);
    DEBUG(TEST_LOGGER, "Called: mock.asic.id=%s", mock->asic->id);
    for (int i = 0; i < mock->asic->addr_dim; i++) {
        DEBUG(TEST_LOGGER, "Called: mock.asic.addr[%d]=%d", i, mock->asic->addr_tuple[i]);
    }

    return BXITP_OK;
}

bxitp_error_e port_cb(bxitp_port_p port, void * data, const char * line, uint32_t line_nb) {
    UNUSED(line);
    UNUSED(line_nb);
    CU_ASSERT_PTR_NOT_NULL_FATAL(port);
    CU_ASSERT_PTR_NOT_NULL_FATAL(data);
    struct mock_s * const mock = (struct mock_s *) data;
    mock->port = port;
    DEBUG(TEST_LOGGER, "Called: mock.port.rank=%d", mock->port->rank);
    DEBUG(TEST_LOGGER, "Called: mock.port.oe_asic_id=%s", mock->port->oe_asic_id);
    DEBUG(TEST_LOGGER, "Called: mock.port.oe_rank=%d", mock->port->oe_rank);
    DEBUG(TEST_LOGGER, "Called: mock.port.bw=%s", mock->port->bw);
    DEBUG(TEST_LOGGER, "Called: mock.port.lat=%s", mock->port->lat);
    DEBUG(TEST_LOGGER, "Called: mock.port.nid=%d", mock->port->nid);

    return BXITP_OK;
}

bxitp_error_e err_cb(bxitp_error_e error, const char * errmsg, void * data,
                     const char * line, uint32_t line_nb) {
    UNUSED(line);
    UNUSED(line_nb);
    ERROR(TEST_LOGGER, "rc=%d: %s", error, errmsg);
    if (data != NULL ) {
        struct mock_s * const mock = (struct mock_s *) data;
        mock->rc = error;
        mock->errmsg = errmsg;
    }

    return 0;
}

bxitp_error_e eof_cb(void * data, uint32_t line_nb) {
    DEBUG(TEST_LOGGER, "EOF Callback called");
    if (data != NULL ) {
        struct mock_s * const mock = (struct mock_s *) data;
        mock->total_line_nb = line_nb;
    }
    return BXITP_OK;
}
bxitp_topo_cbs_s cbs = {.topo_cb = topo_cb,
                        .asic_cb = asic_cb,
                        .port_cb = port_cb,
                        .err_cb = err_cb,
                        .eof_cb = eof_cb};


/*
 * Check parser context creation
 */
void test_bad_context(void) {
    errno = 0;
    bxitp_context_p context = bxitp_new(NULL, NULL );
    CU_ASSERT_PTR_NULL(context);
    CU_ASSERT_EQUAL(errno, EINVAL);
    bxitp_free_context(context);

    cbs = (bxitp_topo_cbs_s){.topo_cb = NULL,
           .asic_cb = NULL,
           .port_cb = NULL,
           .err_cb  = NULL,
           .eof_cb  = NULL};
    context = bxitp_new(&cbs, NULL );
    CU_ASSERT_PTR_NULL(context);
    CU_ASSERT_EQUAL(errno, EINVAL);
    bxitp_free_context(context);

    errno = 0;
    cbs = (bxitp_topo_cbs_s){.topo_cb = NULL,
           .asic_cb = asic_cb,
           .port_cb = port_cb,
           .err_cb  = err_cb,
           .eof_cb  = eof_cb};
    context = bxitp_new(&cbs, NULL );
    CU_ASSERT_PTR_NULL(context);
    CU_ASSERT_EQUAL(errno, EINVAL);
    bxitp_free_context(context);

    errno = 0;
    cbs = (bxitp_topo_cbs_s){.topo_cb = topo_cb,
           .asic_cb = NULL,
           .port_cb = port_cb,
           .err_cb  = err_cb,
           .eof_cb  = eof_cb};
    context = bxitp_new(&cbs, NULL );
    CU_ASSERT_PTR_NULL(context);
    CU_ASSERT_EQUAL(errno, EINVAL);
    bxitp_free_context(context);

    errno = 0;
    cbs = (bxitp_topo_cbs_s){.topo_cb = topo_cb,
           .asic_cb = asic_cb,
           .port_cb = NULL,
           .err_cb  = err_cb,
           .eof_cb  = eof_cb};
    context = bxitp_new(&cbs, NULL );
    CU_ASSERT_PTR_NULL(context);
    CU_ASSERT_EQUAL(errno, EINVAL);
    bxitp_free_context(context);

    errno = 0;
    cbs = (bxitp_topo_cbs_s){.topo_cb = topo_cb,
           .asic_cb = asic_cb,
           .port_cb = port_cb,
           .err_cb  = NULL,
           .eof_cb  = eof_cb};
    context = bxitp_new(&cbs, NULL );
    CU_ASSERT_PTR_NULL(context);
    CU_ASSERT_EQUAL(errno, EINVAL);
    bxitp_free_context(context);

    errno = 0;
    cbs = (bxitp_topo_cbs_s){.topo_cb = topo_cb,
           .asic_cb = asic_cb,
           .port_cb = port_cb,
           .err_cb  = err_cb,
           .eof_cb  = NULL};
    context = bxitp_new(&cbs, NULL );
    CU_ASSERT_PTR_NULL(context);
    CU_ASSERT_EQUAL(errno, EINVAL);
    bxitp_free_context(context);

    errno = 0;
    cbs = (bxitp_topo_cbs_s){.topo_cb = topo_cb,
           .asic_cb = asic_cb,
           .port_cb = port_cb,
           .err_cb  = err_cb,
           .eof_cb  = eof_cb};
    context = bxitp_new(&cbs, NULL );
    CU_ASSERT_PTR_NOT_NULL(context);
    CU_ASSERT_EQUAL(errno, 0);
    CU_ASSERT_PTR_NOT_NULL(context->cbs);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->cbs->topo_cb, (long long) topo_cb);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->cbs->asic_cb, (long long) asic_cb);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->cbs->port_cb, (long long) port_cb);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->cbs->err_cb, (long long) err_cb);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->cbs->eof_cb, (long long)eof_cb);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->data, NULL);
    bxitp_free_context(context);

    errno = 0;
    struct mock_s mock;
    context = bxitp_new(&cbs, &mock);
    CU_ASSERT_PTR_NOT_NULL(context);
    CU_ASSERT_EQUAL(errno, 0);
    CU_ASSERT_PTR_NOT_NULL(context->cbs);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->cbs->topo_cb, (long long) topo_cb);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->cbs->asic_cb, (long long) asic_cb);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->cbs->port_cb, (long long) port_cb);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->cbs->err_cb, (long long) err_cb);
    CU_ASSERT_PTR_EQUAL_FATAL((long long) context->cbs->eof_cb, (long long) eof_cb);
    CU_ASSERT_PTR_EQUAL_FATAL(context->data, &mock);
    bxitp_free_context(context);
}

/*
 * Check parsing bad file
 */
void test_bad_file(void) {
    bxitp_context_p context = bxitp_new(&cbs, NULL );
    CU_ASSERT_EQUAL(errno, 0);
    int rc = bxitp_parse(context, NULL );
    CU_ASSERT_TRUE(rc != 0);
    CU_ASSERT_EQUAL(errno, EINVAL);
    bxitp_free_context(context);
}

void test_comment(void) {
    bxitp_context_p context = bxitp_new(&cbs, NULL );
    char * header = "# A comment";
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    assert(fakedfile != NULL);
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    bxitp_free_context(context);
    fclose(fakedfile);

    context = bxitp_new(&cbs, NULL );
    header = "    # A comment";
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    assert(fakedfile != NULL);
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    bxitp_free_context(context);
    fclose(fakedfile);

    context = bxitp_new(&cbs, NULL );
    header = "    ";
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    assert(fakedfile != NULL);
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    bxitp_free_context(context);
    fclose(fakedfile);

    context = bxitp_new(&cbs, NULL );
    header = "    \n";
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    assert(fakedfile != NULL);
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    bxitp_free_context(context);
    fclose(fakedfile);

    context = bxitp_new(&cbs, NULL );
    header = "\n";
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    assert(fakedfile != NULL);
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    bxitp_free_context(context);
    fclose(fakedfile);

    context = bxitp_new(&cbs, NULL );
    header = "\n\n\n";
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    assert(fakedfile != NULL);
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_unknown_token(void) {
    struct mock_s mock;
    mock.topo = NULL;
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    char * header = "DUMMY TEST";
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    assert(fakedfile != NULL);
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    fclose(fakedfile);
}

void test_badpgft_topo(void) {
    struct mock_s mock;
    mock.topo = NULL;
    char * header = "PGFT doliks=[2,3] uplinks=[3,2] interlinks=[1,2]";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    fclose(fakedfile);
}

void test_badpgft_syntax(void) {
    struct mock_s mock;
    mock.topo = NULL;
    char * header = "PGFT downlinks=[2,3 uplinks=[3,2] interlinks=[1,2]";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    mock.topo = NULL;
    fclose(fakedfile);
    header = "PGFT downlinks=[wrong,3] uplinks=[3,2] interlinks=[1,2]";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    fclose(fakedfile);
}

void test_badpgft_topo_zero(void) {
    struct mock_s mock;
    mock.topo = NULL;
    char * header = "PGFT downlinks=[2,0] uplinks=[3,2] interlinks=[1,2]";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    fclose(fakedfile);

    mock.topo = NULL;
    header = "PGFT downlinks=[2,3] uplinks=[0,2] interlinks=[1,2]";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    fclose(fakedfile);

    mock.topo = NULL;
    header = "PGFT downlinks=[2,3] uplinks=[3,2] interlinks=[1,0]";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    fclose(fakedfile);
}

void test_pgft_header_multidim(void) {
    struct mock_s mock;
    mock.topo = NULL;
    char * header = "PGFT downlinks=[1,2] uplinks=[3,4] interlinks=[5,6]";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    CU_ASSERT_PTR_NOT_NULL_FATAL(mock.topo)
    CU_ASSERT_EQUAL(mock.topo->family, BXITP_PGFT);
    bxitp_pgft_p pgft = mock.topo->family_specific_data;
    CU_ASSERT_EQUAL(2, pgft->dim);
    CU_ASSERT_EQUAL(1, pgft->downlinks[0]);
    CU_ASSERT_EQUAL(2, pgft->downlinks[1]);
    CU_ASSERT_EQUAL(3, pgft->uplinks[0]);
    CU_ASSERT_EQUAL(4, pgft->uplinks[1]);
    CU_ASSERT_EQUAL(5, pgft->interlinks[0]);
    CU_ASSERT_EQUAL(6, pgft->interlinks[1]);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    fclose(fakedfile);
}

void test_pgft_header_multidim_space(void) {
    struct mock_s mock;
    mock.topo = NULL;
    char * header = "PGFT downlinks=[1, 2] uplinks=[3, 4] interlinks=[5, 6]";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    CU_ASSERT_PTR_NOT_NULL_FATAL(mock.topo)
    CU_ASSERT_EQUAL(mock.topo->family, BXITP_PGFT);
    bxitp_pgft_p pgft = mock.topo->family_specific_data;
    CU_ASSERT_EQUAL(2, pgft->dim);
    CU_ASSERT_EQUAL(1, pgft->downlinks[0]);
    CU_ASSERT_EQUAL(2, pgft->downlinks[1]);
    CU_ASSERT_EQUAL(3, pgft->uplinks[0]);
    CU_ASSERT_EQUAL(4, pgft->uplinks[1]);
    CU_ASSERT_EQUAL(5, pgft->interlinks[0]);
    CU_ASSERT_EQUAL(6, pgft->interlinks[1]);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    fclose(fakedfile);
}

void test_pgft_header_singledim(void) {
    struct mock_s mock;
    mock.topo = NULL;
    char * header = "PGFT downlinks=[2] uplinks=[1] interlinks=[3]";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    CU_ASSERT_PTR_NOT_NULL_FATAL(mock.topo)
    CU_ASSERT_EQUAL(mock.topo->family, BXITP_PGFT);
    bxitp_pgft_p pgft = mock.topo->family_specific_data;
    CU_ASSERT_EQUAL(1, pgft->dim);
    CU_ASSERT_EQUAL(2, pgft->downlinks[0]);
    CU_ASSERT_EQUAL(1, pgft->uplinks[0]);
    CU_ASSERT_EQUAL(3, pgft->interlinks[0]);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    fclose(fakedfile);
}

void test_pgft_header_nodim(void) {
    struct mock_s mock;
    mock.topo = NULL;
    // A single vertice is represented by PGFT(0;;;)
    char * header = "PGFT downlinks=[] uplinks=[] interlinks=[]";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    CU_ASSERT_PTR_NOT_NULL_FATAL(mock.topo)
    CU_ASSERT_EQUAL(mock.topo->family, BXITP_PGFT);
    bxitp_pgft_p pgft = mock.topo->family_specific_data;
    CU_ASSERT_EQUAL(0, pgft->dim);
    bxitp_free_context(context);
    bxitp_free_topo(mock.topo);
    fclose(fakedfile);
}

void test_pgft_bad_asic(void) {
    struct mock_s mock;
    char * header = "ASIC radix \"id\" (addr)";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_asic_badradix(void) {
    struct mock_s mock;
    char * header = "ASIC 2.3 \"id\" (addr)";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_asic_badid(void) {
    struct mock_s mock;
    char * header = "ASIC 23 id\" (addr)";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_asic_badaddr(void) {
    struct mock_s mock;
    char * header = "ASIC 23 \"foo\" (1,3";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_asic(void) {
    struct mock_s mock;
    mock.asic = NULL;
    char * header = "ASIC 23 \"foo\" (1,3)";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    CU_ASSERT_PTR_NOT_NULL_FATAL(mock.asic)
    CU_ASSERT_EQUAL(mock.asic->radix, 23);
    CU_ASSERT_STRING_EQUAL(mock.asic->id, "foo");
    CU_ASSERT_EQUAL(mock.asic->addr_dim, 2);
    CU_ASSERT_EQUAL(mock.asic->addr_tuple[0], 1);
    CU_ASSERT_EQUAL(mock.asic->addr_tuple[1], 3);

    bxitp_free_context(context);
    bxitp_free_asic(mock.asic);
    fclose(fakedfile);
}

void test_pgft_bad_port(void) {
    struct mock_s mock;
    char * header = "[oeo ";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_port_badrank(void) {
    struct mock_s mock;
    char * header = "[oeo] ";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);

    header = "[3eo] ";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_port_negrank(void) {
    struct mock_s mock;
    char * header = "[-4] ";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_port_badoeid(void) {
    struct mock_s mock;
    char * header = "[4] missing_double_quote ";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_port_badoerank(void) {
    struct mock_s mock;
    char * header = "[4] \"oeid\"[missing_bracket";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_port_badbw(void) {
    struct mock_s mock;
    char * header = "[4] \"portid\"[2] bw?\"dummy\"0 lat=\"0\" nid=2";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);

    mock.rc = BXITP_OK;
    header = "[4] \"portid\"[2] bw=\" lat=\"0\" nid=2";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_port_badlat(void) {
    struct mock_s mock;
    char * header = "[4] \"portid\"[2] bw=\"\" lat?\"0\" nid=2";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);

    mock.rc = BXITP_OK;
    header = "[4] \"portid\"[2] bw=\"3\" lat=\" nid=2";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_port_badnid(void) {
    struct mock_s mock;
    char * header = "[4] \"portid\"[2] bw=\"0\" lat=\"3\" nid?2";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);

    mock.rc = BXITP_OK;
    header = "[4] \"portid\"[2] bw=\"4\" lat=\"2\" nid=2.3";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);

    mock.rc = BXITP_OK;
    header = "[4] \"portid\"[2] bw=\"3\" lat=\"1\" nid=-2";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);

    mock.rc = BXITP_OK;
    header = "[4] \"portid\"[2] bw=\"3\" lat=\"1\" nid=65537";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_ABORT);
    CU_ASSERT_EQUAL(mock.rc, BXITP_BAD_FORMAT_ERROR);
    bxitp_free_context(context);
    fclose(fakedfile);
}

void test_pgft_port(void) {
    struct mock_s mock;
    mock.port = NULL;
    char * header = "[4] \"portid\"[2] bw=\"3\" lat=\"9\" nid=12 ";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    CU_ASSERT_PTR_NOT_NULL_FATAL(mock.port);
    CU_ASSERT_EQUAL(mock.port->rank, 4);
    CU_ASSERT_STRING_EQUAL(mock.port->oe_asic_id, "portid");
    CU_ASSERT_EQUAL(mock.port->oe_rank, 2);
    CU_ASSERT_STRING_EQUAL(mock.port->bw, "3");
    CU_ASSERT_STRING_EQUAL(mock.port->lat, "9");
    CU_ASSERT_EQUAL(mock.port->nid, 12);
    bxitp_free_context(context);
    bxitp_free_port(mock.port);
    fclose(fakedfile);

    mock.rc = BXITP_OK;
    header = bxistr_new("[1]\t \"portid\"[2]\t bw=\"3\"\t lat=\"4\"\t nid=%d\t",
                                  BXITP_UNDEFINED_NID);
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    CU_ASSERT_PTR_NOT_NULL_FATAL(mock.port);
    CU_ASSERT_EQUAL(mock.port->rank, 1);
    CU_ASSERT_STRING_EQUAL(mock.port->oe_asic_id, "portid");
    CU_ASSERT_EQUAL(mock.port->oe_rank, 2);
    CU_ASSERT_STRING_EQUAL(mock.port->bw, "3");
    CU_ASSERT_STRING_EQUAL(mock.port->lat, "4");
    CU_ASSERT_EQUAL(mock.port->nid, BXITP_UNDEFINED_NID);
    bxitp_free_context(context);
    bxitp_free_port(mock.port);
    fclose(fakedfile);
    BXIFREE(header);

}

void test_eof(void) {
    struct mock_s mock;
    char * header = "\n";
    bxitp_context_p context = bxitp_new(&cbs, &mock);
    FILE* fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    int rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    CU_ASSERT_EQUAL(mock.total_line_nb, 1);
    bxitp_free_context(context);
    fclose(fakedfile);

    header = "\n\n";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    CU_ASSERT_EQUAL(mock.total_line_nb, 2);
    bxitp_free_context(context);
    fclose(fakedfile);

    header = "# Comment\n# Another Comment\n";
    context = bxitp_new(&cbs, &mock);
    fakedfile = fmemopen(header, BXISTR_BYTES_NB(header), "r");
    rc = bxitp_parse(context, fakedfile);
    CU_ASSERT_EQUAL(rc, BXITP_OK);
    CU_ASSERT_EQUAL(mock.total_line_nb, 2);
    bxitp_free_context(context);
    fclose(fakedfile);
}

