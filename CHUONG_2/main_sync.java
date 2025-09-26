import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;

// ==================== LỚP CƠ BẢN ====================
class ResourcesExploiter {
    private int rsc;

    public ResourcesExploiter(int n) {
        rsc = n;
    }

    public void setRsc(int n) {
        rsc = n;
    }

    public int getRsc() {
        return rsc;
    }

    // phương thức tăng không đồng bộ
    public void exploit() {
        setRsc(getRsc() + 1);
    }
}

// ==================== WITHOUT SYNC ====================
class ThreadedWorkerWithoutSync extends Thread {
    private ResourcesExploiter rExp;

    public ThreadedWorkerWithoutSync(ResourcesExploiter rExp) {
        this.rExp = rExp;
    }

    @Override
    public void run() {
        for (int i = 0; i < 1000; i++) {
            rExp.exploit();
        }
    }
}

// ==================== WITH SYNC ====================
class ResourcesExploiterWithSync extends ResourcesExploiter {
    public ResourcesExploiterWithSync(int n) {
        super(n);
    }

    @Override
    public synchronized void exploit() {
        setRsc(getRsc() + 1);
    }
}

class ThreadedWorkerWithSync extends Thread {
    private ResourcesExploiterWithSync rExp;

    public ThreadedWorkerWithSync(ResourcesExploiterWithSync rExp) {
        this.rExp = rExp;
    }

    @Override
    public void run() {
        for (int i = 0; i < 1000; i++) {
            rExp.exploit();
        }
    }
}

// ==================== WITH LOCK ====================
class ResourcesExploiterWithLock extends ResourcesExploiter {
    private ReentrantLock lock;

    public ResourcesExploiterWithLock(int n) {
        super(n);
        lock = new ReentrantLock();
    }

    @Override
    public void exploit() {
        try {
            if (lock.tryLock(10, TimeUnit.SECONDS)) {
                try {
                    setRsc(getRsc() + 1);
                } finally {
                    lock.unlock();
                }
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}

class ThreadedWorkerWithLock extends Thread {
    private ResourcesExploiterWithLock rExp;

    public ThreadedWorkerWithLock(ResourcesExploiterWithLock rExp) {
        this.rExp = rExp;
    }

    @Override
    public void run() {
        for (int i = 0; i < 1000; i++) {
            rExp.exploit();
        }
    }
}

// ==================== MAIN ====================
public class Main {
    public static void main(String[] args) {
        // -------- WITHOUT SYNC --------
        ResourcesExploiter resource1 = new ResourcesExploiter(0);
        ThreadedWorkerWithoutSync w1 = new ThreadedWorkerWithoutSync(resource1);
        ThreadedWorkerWithoutSync w2 = new ThreadedWorkerWithoutSync(resource1);
        ThreadedWorkerWithoutSync w3 = new ThreadedWorkerWithoutSync(resource1);

        w1.start(); w2.start(); w3.start();
        try { w1.join(); w2.join(); w3.join(); } catch (InterruptedException e) { e.printStackTrace(); }
        System.out.println("Without Sync: " + resource1.getRsc());

        // -------- WITH SYNC --------
        ResourcesExploiterWithSync resource2 = new ResourcesExploiterWithSync(0);
        ThreadedWorkerWithSync s1 = new ThreadedWorkerWithSync(resource2);
        ThreadedWorkerWithSync s2 = new ThreadedWorkerWithSync(resource2);
        ThreadedWorkerWithSync s3 = new ThreadedWorkerWithSync(resource2);

        s1.start(); s2.start(); s3.start();
        try { s1.join(); s2.join(); s3.join(); } catch (InterruptedException e) { e.printStackTrace(); }
        System.out.println("With Sync: " + resource2.getRsc());

        // -------- WITH LOCK --------
        ResourcesExploiterWithLock resource3 = new ResourcesExploiterWithLock(0);
        ThreadedWorkerWithLock l1 = new ThreadedWorkerWithLock(resource3);
        ThreadedWorkerWithLock l2 = new ThreadedWorkerWithLock(resource3);
        ThreadedWorkerWithLock l3 = new ThreadedWorkerWithLock(resource3);

        l1.start(); l2.start(); l3.start();
        try { l1.join(); l2.join(); l3.join(); } catch (InterruptedException e) { e.printStackTrace(); }
        System.out.println("With Lock: " + resource3.getRsc());
    }
}

