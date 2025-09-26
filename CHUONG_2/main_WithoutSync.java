// Lớp tài nguyên dùng chung
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

    // Phương thức khai thác tài nguyên (tăng giá trị lên 1)
    public void exploit() {
        setRsc(getRsc() + 1);
    }
}

// Lớp worker chạy bằng luồng
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

// Lớp chính để chạy chương trình
public class Main {
    public static void main(String[] args) {
        // Tạo tài nguyên chung ban đầu = 0
        ResourcesExploiter resource = new ResourcesExploiter(0);

        // Tạo 3 worker
        ThreadedWorkerWithoutSync worker1 = new ThreadedWorkerWithoutSync(resource);
        ThreadedWorkerWithoutSync worker2 = new ThreadedWorkerWithoutSync(resource);
        ThreadedWorkerWithoutSync worker3 = new ThreadedWorkerWithoutSync(resource);

        // Khởi động 3 luồng
        worker1.start();
        worker2.start();
        worker3.start();

        try {
            // Chờ các luồng kết thúc
            worker1.join();
            worker2.join();
            worker3.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        // In ra giá trị cuối cùng của tài nguyên
        System.out.println("Final value of resource: " + resource.getRsc());
    }
}

