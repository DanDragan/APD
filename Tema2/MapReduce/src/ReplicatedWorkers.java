import java.util.HashMap;

/**
 * Clasa ce reprezinta o solutie partiala pentru problema de rezolvat. Aceste
 * solutii partiale constituie task-uri care sunt introduse in workpool.
 */
abstract class PartialSolution {
	abstract public void go();
}

/**
 * Clasa ce reprezinta un thread worker.
 */
class Worker extends Thread {
	WorkPool wp;
	HashMap<String, Float> frequencies;

	public Worker(WorkPool workpool) {
		this.wp = workpool;
	}

	/**
	 * Procesarea unei solutii partiale. Aceasta poate implica generarea unor
	 * noi solutii partiale care se adauga in workpool folosind putWork(). Daca
	 * s-a ajuns la o solutie finala, aceasta va fi afisata.
	 */
	void processPartialSolution(PartialSolution ps) {
		ps.go();
		/*
		 * Mapeaza fiecare vector partial la numele fisierului
		 */
		if (ps instanceof Map) {
			
			String file;
			Map task = (Map) ps;
			file = task.getFile();
			Main.mapHashToFile(task.getWords(), file);
		} 
		/*
		 * Mapeaza gardul de asemanare dintre fisierul selectat
		 * si orice alt fisier la numele acelui fisier
		 */
		else if (ps instanceof Reduce) {

			Reduce task = (Reduce) ps;
			String file;
			file = task.getFile();
			Main.mapSymmetryToFile(file, task.getSim());
		}
	}

	HashMap<String, Float> getFrequencies() {
		return frequencies;
	}

	public void run() {
		while (true) {
			PartialSolution ps = wp.getWork();
			if (ps == null)
				break;

			processPartialSolution(ps);
		}
	}
}