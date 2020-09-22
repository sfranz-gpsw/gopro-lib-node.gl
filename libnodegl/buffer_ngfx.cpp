/*
 * Copyright 2019 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "buffer_ngfx.h"
#include "gctx_ngfx.h"
#include "log.h"

struct buffer *ngli_buffer_ngfx_create(struct gctx *gctx)
{ TODO();
    return NULL;
}

int ngli_buffer_ngfx_init(struct buffer *s, int size, int usage)
{ TODO();
    return 0;
}

int ngli_buffer_ngfx_upload(struct buffer *s, const void *data, int size)
{ TODO();
    return 0;
}

int ngli_buffer_ngfx_download(struct buffer* s, void* data, uint32_t size, uint32_t offset) { TODO();
    return 0;
}

int ngli_buffer_ngfx_map(struct buffer *s, int size, uint32_t offset, void** data)
{ TODO();
    return 0;
}

void ngli_buffer_ngfx_unmap(struct buffer* s) { TODO();

}

void ngli_buffer_ngfx_freep(struct buffer **sp) { TODO();

}
