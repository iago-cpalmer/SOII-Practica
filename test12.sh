public class GraphAdjacencyLists {
	private int size_vertices;
	private LinkedList<Integer>[] graph;
	// Crea un graf amb size_vertices vèrtexs
	public GraphAdjacencyLists(int size_vertices) {
		this.size_vertices = size_vertices;
		this.graph = new LinkedList[this.size_vertices];
		
		for(int i = 0; i< size_vertices; i++) {
			this.graph[i] = new LinkedList<>();
		}
	}
	// Uneix els vèrtexs x i y amb una aresta
	public void putEdge(int x, int y) {
		if (!this.graph[x].contains((Integer) y)) {
			this.graph[x].add(y);
		}
	}
	// Elimina l’aresta entre els vèrtexs x i y
	public void removeEdge(int x, int y) {
		this.graph[x].remove(y);
	}
	
	private class GraphIterator implements Iterator {
		Iterator listIterator;
		// Prepara l’iterador per a recórrer els successors del vèrtex x
		public GraphIterator(int x) {
			listIterator = graph[x].iterator();
		}
		// Retorna si el vèrtex x té més successors
		public boolean hasNext() {
			return listIterator.hasNext();
		}
		// Retorna el següent successor del vèrtex x
		public Object next() {
			return listIterator.next();
		}
	}
}