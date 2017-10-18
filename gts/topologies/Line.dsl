type Line {
	description = "Line topology with selected locations" 
  
	host { 
		id="h1"
		location="BRA" 
		port { id="port1" }
	}
	
	host {
		id="h2"
		location="PAR" 
		port { id="port1" }
		port { id="port2" }
	} 
	
	host {
		id="h3"
		location="MAD"
		port { id="port1" }
	}
	
	link {
		id="l1"
		port { id="src" } 
		port { id="dst" } 
	} 
 
	link {
		id="l2"
		port { id="src" } 
		port { id="dst" } 
	}
	
	adjacency h1.port1, l1.src
	adjacency h2.port1, l1.dst
	adjacency h2.port2, l2.src
	adjacency h3.port1, l2.dst
	
}	