using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneChanger : MonoBehaviour
{
    public void ChangeSceneByIndex(int sceneIndex)
    {
        // Belirtilen sahne numaras�na g�re sahneye ge�i� yap
        SceneManager.LoadScene(1);
    }
}
