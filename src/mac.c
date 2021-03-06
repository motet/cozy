/*
 * Copyright 2020 RISE Research Institutes of Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
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

#ifdef CONFIG_COZY_MAC

#include <cozy/cose.h>
#include <cozy/common.h>

int cose_mac0_write(cose_crypt_context_t *ctx,
        const uint8_t * pld, const size_t len_pld, 
        uint8_t * obj, size_t * len_obj) 
{

    return COSE_ERROR_NONE;
}

int cose_mac0_read(cose_crypt_context_t * ctx,
        const uint8_t * obj, const size_t len_obj, 
        uint8_t * pld, size_t * len_pld)
{

    return COSE_ERROR_NONE;
}

#endif /* CONFIG_COZY_MAC */
