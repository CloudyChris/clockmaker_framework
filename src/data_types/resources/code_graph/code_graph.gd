# code_graph.gd
@tool
class_name CodeGraph
extends CodeGraphNode

@export var nodes : Array[CodeGraphNode] = []
var node_id_holes : Array[int] = []
var base_node_id : int = 0
#-------------------------------------------------------------------------------
@export var connections : Array[CodeGraphConnection] = []
var connection_id_holes : Array[int] = []
var base_connection_id : int = 0
#-------------------------------------------------------------------------------
var run_order : Array[CodeGraphNode]
#-------------------------------------------------------------------------------
var is_ordered : bool = false
var is_normalized : bool = false
#-------------------------------------------------------------------------------
@export var invalid_nodes : Array[int]
@export var invalid_connections : Array[int]
#-------------------------------------------------------------------------------
func get_node(p_id: int) -> CodeGraphNode:
	return nodes.get(p_id)

func assing_node_id(p_node: CodeGraphNode) -> int:
	var assigned_id : int = -1
	if not node_id_holes.is_empty():
		assigned_id = node_id_holes.pop_back()
	else:
		assigned_id = base_node_id
		base_node_id += 1
	p_node.set_graph_id(assigned_id)
	nodes.set(assigned_id, p_node)
	
	return assigned_id

func remove_node(p_id: int) -> bool:
	if not nodes.has(p_id):
		return false
	nodes.set(p_id, null)
	node_id_holes.append(p_id)
	return true
#-------------------------------------------------------------------------------
func get_connection(p_id: int) -> CodeGraphConnection:
	return connections.get(p_id)

func assing_connection_id(p_connection: CodeGraphConnection) -> int:
	var assigned_id : int = -1
	if not connection_id_holes.is_empty():
		assigned_id = connection_id_holes.pop_back()
	else:
		assigned_id = base_connection_id
		base_connection_id += 1
	p_connection.set_graph_id(assigned_id)
	connections.set(assigned_id, p_connection)
	
	return assigned_id

func remove_connection(p_id: int) -> bool:
	if not connections.has(p_id):
		return false
	connections.set(p_id, null)
	connection_id_holes.append(p_id)
	return true
#-------------------------------------------------------------------------------

## virtual
func get_valid_nodes() -> Array[CodeGraphNode]:
	return []
#-------------------------------------------------------------------------------
func normalize_graph_positions() -> void:
	var normalization_vec : Vector2i = Vector2i(0, 0)
	for node in nodes:
		if node:
			if node.position_start.x < normalization_vec.x:
				normalization_vec.x = node.position_start.x
			if node.position_start.y < normalization_vec.y:
				normalization_vec.y = node.position_start.y
	
	for node in nodes:
		if node:
			node.position_start -= normalization_vec
			node.position_end -= normalization_vec
	
	for connection in connections:
		if connection:
			connection.position_start -= normalization_vec
			connection.position_end -= normalization_vec
	
	is_normalized = true

## everything is ordered as if syncrhonous
func order_graph() -> void:
	run_order.clear()
	var nodes_to_check : Array[CodeGraphNode]
	for node in nodes:
		if node.outputs_connected == 0:
			nodes_to_check.push_back(node)
			run_order.push_back(node)
	
	var node_to_check : CodeGraphNode = null
	while not nodes_to_check.is_empty():
		node_to_check = nodes_to_check.pop_front()
		for input_slot in node_to_check.input_slots:
			if input_slot.active_connection:
				nodes_to_check.push_back(input_slot.active_connection.input_node_slot.parent_node)
				run_order.push_front(input_slot.active_connection.input_node_slot.parent_node)
	
	is_ordered = true
#-------------------------------------------------------------------------------
## virtual, call super first
func validate() -> bool:
	if not super():
		is_valid = false
		return false
	
	invalid_nodes.clear()
	invalid_connections.clear()
	
	for node in nodes:
		if node:
			if not node.validate():
				is_valid = false
				invalid_nodes.append(node.get_graph_id())
	
	for connection in connections:
		if connection:
			if not connection.validate():
				is_valid = false
				invalid_connections.append(connection.get_graph_id())

	return is_valid
#-------------------------------------------------------------------------------
func can_run() -> bool:
	if not super():
		return false
	if run_order.is_empty():
		return false
	if not invalid_nodes.is_empty() or not invalid_connections.is_empty():
		return false
	return true

## run the graph
func node_run() -> bool:
	var success : bool = true
	for node in run_order:
		success = node.run()
	
	return success

func reset() -> void:
	super()
	for node in nodes:
		if node:
			node.reset()
