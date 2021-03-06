/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/common/priority_queue.h>
#include <proof_helpers/make_common_data_structures.h>

/**
 * Runtime: 8s
 */
void aws_priority_queue_init_static_harness() {

    /* data structure */
    struct aws_priority_queue *queue;

    /* parameters */
    size_t item_size;
    size_t initial_item_allocation;
    size_t len;

    /* assumptions */
    ASSUME_VALID_MEMORY(queue);
    __CPROVER_assume(initial_item_allocation > 0 && initial_item_allocation <= MAX_INITIAL_ITEM_ALLOCATION);
    __CPROVER_assume(item_size > 0 && item_size <= MAX_ITEM_SIZE);
    __CPROVER_assume(!aws_mul_size_checked(initial_item_allocation, item_size, &len));

    /* perform operation under verification */
    uint8_t *raw_array = bounded_malloc(len);
    aws_priority_queue_init_static(queue, raw_array, initial_item_allocation, item_size, nondet_compare);

    /* assertions */
    assert(aws_priority_queue_is_valid(queue));
    assert(queue->container.alloc == NULL);
    assert(queue->container.item_size == item_size);
    assert(queue->container.length == 0);
    assert(queue->container.current_size == initial_item_allocation * item_size);
    assert_bytes_match((uint8_t *)queue->container.data, raw_array, len);
}
