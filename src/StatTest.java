class StatTest implements IStatTest { 
	ArrayList<StatResult> score(byte[] data) { 
		StatResult result = new StatResult();
		result.name = "mock";
		result.score = 0.5;
		return new ArrayList<StatResult>() { result } 
	}

		


}

interface IStatTest { 
	ArrayList<StatResult> score(byte[] data);
}


class StatResult { 
	String name;
	float score;

	StatResult() { }
}



