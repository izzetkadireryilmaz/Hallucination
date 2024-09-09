using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneChanger : MonoBehaviour
{
    public void ChangeSceneByIndex(int sceneIndex)
    {
        // Belirtilen sahne numarasýna göre sahneye geçiþ yap
        SceneManager.LoadScene(1);
    }
}
