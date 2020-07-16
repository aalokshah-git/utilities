/*******************************************************************
 * Copyright [2018] [Aalok Shah, Nisarg Prajapati]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *********************************************************************/

#include  <stdint.h>
#include  <net/if.h>
#include  <stdio.h>
#include  "cps_api_object.h"
#include  "dell-base-switch-element.h"
#include  "dell-base-qos.h"
#include  "cps_class_map.h"
#include  "cps_api_object_key.h"

char *interface[8] =
{
	"e101-00-0",
	"e101-01-0",
	"e101-02-0",
	"e101-03-0",
	"e101-04-0",
	"e101-05-0",
	"e101-06-0",
	"e101-07-0",
};

bool cps_get_temp()
{
	// Initialize the Get Object
	cps_api_get_params_t gp; 
	if (cps_api_get_request_init(&gp) !=  cps_api_ret_code_OK)
	{
		printf("Failed to initialize get-request\n");
		return false;
	}

	//Create a new object and append it to get requests filter object list
	cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
	if(obj == NULL)
	{
		cps_api_get_request_close(&gp);
		return false;
	}

	//Create, initialize and set the key for object
	cps_api_key_t   key;
	cps_api_key_from_attr_with_qual(&key, BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY, cps_api_qualifier_TARGET);
	cps_api_object_set_key(obj,&key);

	if (cps_api_get(&gp) == cps_api_ret_code_OK)
	{
		size_t mx = cps_api_object_list_size(gp.list);
		size_t ix;
		cps_api_object_t response_obj; 
		cps_api_object_attr_t temp;

		for(ix = 0; ix < mx; ++ix) 
		{ 
			response_obj = cps_api_object_list_get(gp.list,ix);
			temp = cps_api_object_attr_get(response_obj, BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_TEMPERATURE); 
			printf("Temperature is %d\n",cps_api_object_attr_data_u32(temp));
		}
	}

	//Close the get request
	cps_api_get_request_close(&gp);
	return   true;
}

bool cps_get_fdb_size()
{
	//Initialize the Get Object
	cps_api_get_params_t gp; 
	if (cps_api_get_request_init(&gp) !=  cps_api_ret_code_OK)
	{
		printf("Failed to initialize get-request\n");
		return false;
	}

	//Create a new object and append it to get requests filter object list
	cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
	if(obj == NULL)
	{
		cps_api_get_request_close(&gp);
		return false;
	}

	//Create, initialize and set the key for object
	cps_api_key_t key;
	cps_api_key_from_attr_with_qual(&key, BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY, cps_api_qualifier_TARGET);
	cps_api_object_set_key(obj,&key);

	if (cps_api_get(&gp)==cps_api_ret_code_OK)
	{
		size_t mx = cps_api_object_list_size(gp.list);
		size_t ix;
		cps_api_object_t response_obj; 
		cps_api_object_attr_t table_size;

		for(ix = 0; ix < mx; ++ix) 
		{ 
			response_obj = cps_api_object_list_get(gp.list,ix);
			table_size = cps_api_object_attr_get(response_obj, BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_BRIDGE_TABLE_SIZE); 
			printf("FDB Table Size is %d\n",cps_api_object_attr_data_u32(table_size));
		}
	}

	//Close the get request
	cps_api_get_request_close(&gp);
	return true;
}

bool cps_get_queues_per_port(char **string)
{
	//Initialize the Get Object
	cps_api_get_params_t gp; 
	if (cps_api_get_request_init(&gp) !=  cps_api_ret_code_OK)
	{
		printf("Failed to initialize get request\n");
		return false;
	}

	// Create a new object and append it to get request's filter object-list
	cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
	if(obj == NULL)
	{
		cps_api_get_request_close(&gp);
		return false;
	}

	int index = if_nametoindex(*string);
	// Create, initialize and set the key for object
	cps_api_key_from_attr_with_qual(cps_api_object_key(obj), BASE_QOS_PORT_EGRESS_OBJ, cps_api_qualifier_TARGET);
	cps_api_set_key_data(obj, BASE_QOS_PORT_EGRESS_PORT_ID,	cps_api_object_ATTR_T_U32, &index, sizeof(uint32_t));
	cps_api_object_attr_add_u32(obj, BASE_QOS_PORT_EGRESS_NUM_QUEUE, 0);


	if (cps_api_get(&gp)==cps_api_ret_code_OK)
	{
		size_t mx = cps_api_object_list_size(gp.list);
		size_t ix;
		cps_api_object_t response_obj; 
		cps_api_object_attr_t queue_num = NULL;
		uint8_t num_queue = 0;

		for(ix = 0; ix < mx; ++ix) 
		{ 
			response_obj = cps_api_object_list_get(gp.list,ix);
			queue_num = cps_api_object_attr_get(response_obj, BASE_QOS_PORT_EGRESS_NUM_QUEUE);
			if (queue_num == NULL)
			{
				return false;
			}
			num_queue = ((uint8_t *)cps_api_object_attr_data_bin(queue_num))[0]; 
			printf("Queue per Port for port %d ==> %d\n",index, num_queue);
		}
	}
	else
	{
		cps_api_get_request_close(&gp);
		return false;
	}

	cps_api_get_request_close(&gp);
	return true;
}


int main()
{
	bool ret;
	int i;

	printf("=================================================TEMPERATURE=================================================\n");
	ret = cps_get_temp();
	if (ret == false)
	{
		printf("Failed to fetch Temperature\n");
	}

	printf("=================================================FDB-TABLE-SIZE=================================================\n");
	ret = cps_get_fdb_size();
	if (ret == false)
	{
		printf("Failed to fetch FDB Size\n");
	}

	printf("=================================================QUEUES-PER-PORT=================================================\n");
	for(i = 0; i<=7; i++)
	{
		ret = cps_get_queues_per_port(&interface[i]);
		if (ret == false)
		{
			printf("Failed to fetch queues allocated to port for tap-interface %s\n", interface[i]);
		}
	}
	return 0;
}

