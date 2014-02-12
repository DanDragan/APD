import java.util.HashMap;
import java.util.Vector;
import java.io.File;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.FileWriter;

public class Main {
	
	static HashMap<String, Vector<HashMap<String, Integer>>> map = 
			new HashMap<String, Vector<HashMap<String, Integer>>>();
	static HashMap<String, HashMap<String, Double>> fmap = 
			new HashMap<String, HashMap<String, Double>>();;
	static HashMap<String, Double> simMap = 
			new HashMap<String, Double>();
	static boolean firstIsOver = false;
	private static String inputFile = "", outputFile = "";
	private static String selectedFile = "";
	private static int dimFragments, numDocuments;
	private static float limit = 0;
	private static String[] docs;

	/*
	 * adauga HashMap-uri partiale la un Vector 
	 * ce va fi mapat la numele fisierului
	 */
	static void mapHashToFile(HashMap<String, Integer> words, String file) {
		map.get(file).add(words);
	}
	/*
	 * mapeaza asemanarile dintre documentul selectat 
	 * si celelalte documente la numele acelui document
	 */
	static void mapSymmetryToFile(String file, double sim) {
		simMap.put(file, sim);
	}

	/*
	 * mapeaza HashMap-ul ce contine frecventele 
	 * cuvintelor fiecarui fisier la numele fisierului
	 */
	static void mapFreqToFile(HashMap<String, Double> freq, String file) {
		fmap.put(file, freq);
	}
	
	/*
	 * Citeste din fisier
	 */
	public static void readFile() throws Exception {
		BufferedReader br;
		br = new BufferedReader(new FileReader(inputFile));
		
		selectedFile = br.readLine();
		dimFragments = Integer.parseInt(br.readLine());
		limit = Float.parseFloat(br.readLine());
		numDocuments = Integer.parseInt(br.readLine());

		docs = new String[numDocuments];
		for (int i = 0; i < numDocuments; i++)
			docs[i] = br.readLine();
		br.close();
	}
	
	/*
	 * Scrie in fisier
	 */
	public static void writeFile() throws Exception {
		BufferedWriter bw = new BufferedWriter(new FileWriter(outputFile));
		bw.write("Rezultate pentru: (" + selectedFile + ")\n\n");
		for (int i = 0; i < numDocuments; i++) {
			if (!docs[i].equals(selectedFile)) {
				double sim = simMap.get(docs[i]) / 100 ;
				String ssim = new String("" + sim);
				int pos = ssim.indexOf('.');
				if (sim >= limit) {
					bw.write(docs[i] + " (" + ssim.subSequence(0, pos + 4) + "%)\n");
				}
			}
		}
		bw.close();
	}

	public static void main(String[] args) {

		int numThreads = 0;

		if (args.length != 3) {
			System.out.println("Numar insuficient de argumente");
		} else {
			numThreads = Integer.parseInt(args[0]);
			inputFile = args[1];
			outputFile = args[2];
		}

		try {

			readFile();
			
			WorkPool wp = new WorkPool(numThreads);
			for (int i = 0; i < numDocuments; i++) {
				map.put(docs[i], new Vector<HashMap<String, Integer>>());
				File file = new File(docs[i]);
				long size = file.length();
				/*
				 * impartire fisier pe fragmente de dimensiune dimFragments
				 */
				for (long j = 0; j < size; j += dimFragments) {
					Map task;
					if(j + dimFragments < size) {
						task = new Map(j, j + dimFragments, docs[i]);
					} else {
						task = new Map(j, size - 1, docs[i]);
					}
					wp.putWork(task);
				}
			}
			
			Worker[] mworkers = new Worker[numThreads];
			for (int i = 0; i < numThreads; i++)
				mworkers[i] = new Worker(wp);
			/*
			 * Se pornesc workerii ce fac operatia de Map
			 */
			for (int i = 0; i < numThreads; i++)
				mworkers[i].start();
			
			/*
			 * Se asteapta terminarea executiei tuturor workerilor
			 */
			for (int i = 0; i < numThreads; i++)
				mworkers[i].join();

			for (int i = 0; i < numDocuments; i++) {
				Reduce task = new Reduce(map.get(docs[i]), docs[i],
						selectedFile);
				wp.putWork(task);
			}
			
			Worker[] rworkers = new Worker[numThreads];
			for (int i = 0; i < numThreads; i++) {
				rworkers[i] = new Worker(wp);
			}
			/*
			 * Se pornesc workerii ce fac prima operatie de Reduce, adica
			 * combina HashMap-urile partiale intr-un singur HashMap
			 */
			for (int i = 0; i < numThreads; i++)
				rworkers[i].start();
			/*
			 * Se asteapta terminarea executiei tuturor workerilor
			 */
			for (int i = 0; i < numThreads; i++)
				rworkers[i].join();
			/*
			 * Se marcheaza terminarea operatiei de combinare
			 */
			firstIsOver = true;
			for (int i = 0; i < numDocuments; i++) {
				Reduce task = new Reduce(map.get(docs[i]), docs[i],
						selectedFile);
				wp.putWork(task);
			}

			rworkers = new Worker[numThreads];
			for (int i = 0; i < numThreads; i++)
				rworkers[i] = new Worker(wp);
			/*
			 * Se pornesc workerii ce executa a 2-a operatie de Reduce,
			 * adica determinarea asemanarilor dintre fiecare fisier
			 * si fisierul selectat
			 */
			for (int i = 0; i < numThreads; i++)
				rworkers[i].start();

			for (int i = 0; i < numThreads; i++)
				rworkers[i].join();
			
			writeFile();
			
		} catch (Exception e) {
			e.printStackTrace();
		}

	}
}
